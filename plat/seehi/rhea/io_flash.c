#include <assert.h>
#include <string.h>

#include <platform_def.h>

#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_storage.h>
#include <lib/utils.h>
#include <io_flash.h>
#include <io_flash.h>
#include <nand_flash.h>
#include <lib/mmio.h>

static spi_id_t in_use_spi_id = BOOTSPI_ID;
static bool is_nand_flash = false;
static bool map_mode = true;
static bool flash_quad_output_fast_read = false;

#define READ_ONLY

/* As we need to be able to keep state for seek, only one file can be open
 * at a time. Make this a structure and point to the entity->info. When we
 * can malloc memory we can change this to support more open files.
 */
typedef struct {
	/* Use the 'in_use' flag as any value for base and file_pos could be
	 * valid.
	 */
	int			in_use;
	uintptr_t		base;
	unsigned long long	file_pos;
	unsigned long long	size;
} io_flash_file_state_t;

static io_flash_file_state_t current_io_flash_file = {0};

static int io_flash_dev_open(const uintptr_t dev_spec __unused, io_dev_info_t **dev_info);
static io_type_t io_flash_type(void);
static int io_flash_open(io_dev_info_t *dev_info, const uintptr_t spec,
		io_entity_t *entity);
static int io_flash_seek(io_entity_t *entity, int mode, signed long long offset);
static int io_flash_size(io_entity_t *entity, size_t *length);
static int io_flash_read(io_entity_t *entity, uintptr_t buffer, size_t length,
		size_t *length_read);
#if !defined(READ_ONLY)
static int io_flash_write(io_entity_t *entity, const uintptr_t buffer,
		size_t length, size_t *length_written);
#endif
static int io_flash_close(io_entity_t *entity);
static int io_flash_dev_init(io_dev_info_t *dev_info, const uintptr_t init_params);
static int io_flash_dev_close(io_dev_info_t *dev_info);

static const io_dev_connector_t io_flash_dev_connector = {
	.dev_open = io_flash_dev_open
};

static const io_dev_funcs_t io_flash_dev_funcs = {
	.type = io_flash_type,
	.open = io_flash_open,
	.seek = io_flash_seek,
	.size = io_flash_size,
	.read = io_flash_read,
#if !defined(READ_ONLY)
	.write = io_flash_write,
#else
	.write = NULL,
#endif
	.close = io_flash_close,
	.dev_init = io_flash_dev_init,
	.dev_close = io_flash_dev_close,
};

static io_dev_info_t io_flash_dev_info = {
	.funcs = &io_flash_dev_funcs,
	.info = (uintptr_t)NULL
};

static io_type_t io_flash_type(void)
{
	return IO_TYPE_MEMMAP;
}

static int io_flash_dev_open(const uintptr_t dev_spec __unused, io_dev_info_t **dev_info)
{
	assert(dev_info != NULL);
	*dev_info = &io_flash_dev_info;
	return 0;
}

static int io_flash_dev_init(io_dev_info_t *dev_info, const uintptr_t init_params)
{
	assert(dev_info);
	assert(init_params);
	io_flash_dev_init_spec_t* dev_init_spec = (io_flash_dev_init_spec_t*)init_params;
	in_use_spi_id = dev_init_spec->spi_id;
	if( (in_use_spi_id != BOOTSPI_ID)  || (!is_boot_from_flash()/*rom启动*/&&(dev_init_spec->map_mode==false)/*不使用map模式*/) )
	{
		if(!flash_init(in_use_spi_id, dev_init_spec->clk_div, dev_init_spec->spi_mode, dev_init_spec->flash_model))
			return -1;

		uint8_t flash_id[3];
		flash_read_id(in_use_spi_id, flash_id, 3);
		INFO("SPI_FLASH: Flash ID: 0x%x 0x%x 0x%x.\n\r", flash_id[0], flash_id[1], flash_id[2]);
		
		if(flash_id[1]==0xff || flash_id[2]==0xff || flash_id[1]==0x00 || flash_id[2]==0x00)
		{
			ERROR("SPI_FLASH: Flash ID is invalid.\n\r");
			return -1;
		}

		if(flash_id[0]==0xff || flash_id[0]==0x00)
		{
			INFO("This is NAND flash.\n\r");
			is_nand_flash = true;
		}
		else
		{
			INFO("This is NOR flash.\n\r");
			is_nand_flash = false;
		}
		
		if((dev_init_spec->flash_model != UNKNOWN_FLASH) && !is_nand_flash)
		{
			void flash_set_QE_bit(spi_id_t spi_id);
			flash_set_QE_bit(in_use_spi_id);
			flash_quad_output_fast_read = true;
		}
		INFO("SPI_FLASH: CLK_DIV=%u.\n\r", dev_init_spec->clk_div);
		if(flash_quad_output_fast_read)
			INFO("SPI_FLASH: Read flash by Quad_Fast_Read.\n\r");
		map_mode = false;
	}
	else
	{
		mmio_write_32(BOOTSPI_BASE, 0); //默认就是map模式
		NOTICE("SPI_FLASH: Bootspi is in MAP_MODE.\n\r");
		map_mode = true;
		flash_quad_output_fast_read = false;
	}
	return 0;
}

static int io_flash_dev_close(io_dev_info_t *dev_info)
{
	flash_deinit(BOOTSPI_ID);
	return 0;
}

static int io_flash_open(io_dev_info_t *dev_info __unused, const uintptr_t spec,
		io_entity_t *entity)
{
	assert(spec);
	assert(entity);
	if(current_io_flash_file.in_use != 0)
	{
		WARN("A nor flash device is already active. Close first.\n");
		return -1;
	}

	io_flash_io_open_spec_t* io_open_spec = (io_flash_io_open_spec_t*)spec;
	current_io_flash_file.base = io_open_spec->flash_block.offset;
	current_io_flash_file.size = io_open_spec->flash_block.length;
	current_io_flash_file.file_pos = 0;
	current_io_flash_file.in_use = 1;
	entity->info = (uintptr_t)&current_io_flash_file;
	return 0;
}

static int io_flash_seek(io_entity_t *entity, int mode, signed long long offset)
{
	int result = -1;
	io_flash_file_state_t *fp;

	/* We only support IO_SEEK_SET for the moment. */
	if (mode == IO_SEEK_SET)
	{
		assert(entity != NULL);

		fp = (io_flash_file_state_t *) entity->info;

		/* Assert that new file position is valid */
		assert((offset >= 0) &&
		       ((unsigned long long)offset < fp->size));

		/* Reset file position */
		fp->file_pos = (unsigned long long)offset;
		result = 0;
	}
	else
	{
		WARN("We only support IO_SEEK_SET for the moment.");
	}
	return result;
}

static int io_flash_size(io_entity_t *entity, size_t *length)
{
	assert(entity != NULL);
	assert(length != NULL);

	*length = (size_t)((io_flash_file_state_t *)entity->info)->size;

	return 0;
}

static int io_flash_read(io_entity_t *entity, uintptr_t buffer, size_t length,
		size_t *length_read)
{
	assert(entity);
	assert(length_read);
	assert(buffer);
	unsigned long long pos_after;
	io_flash_file_state_t *fp = (io_flash_file_state_t *) entity->info;

	pos_after = fp->file_pos + length;
	assert((pos_after >= fp->file_pos) && (pos_after <= fp->size));
	if (!map_mode)
	{
		if(is_nand_flash)
			(void)nand_flash_read(in_use_spi_id, (uint32_t)(fp->base + fp->file_pos - FLASH_MAP_BASE), (uint8_t*)buffer, (size_t)length);
		else
		{
			if(flash_quad_output_fast_read)
				flash_read_quad_dma(in_use_spi_id, (uint32_t)(fp->base + fp->file_pos - FLASH_MAP_BASE), (uint8_t*)buffer, (size_t)length);
			else
				flash_read_dma(in_use_spi_id, (uint32_t)(fp->base + fp->file_pos - FLASH_MAP_BASE), (uint8_t*)buffer, (size_t)length);
		}

	}
	else
		(void)memcpy((void*)buffer, (void*)(uintptr_t)(fp->base + fp->file_pos), length);
	*length_read = length;
	fp->file_pos = pos_after;
	return 0;
}

#if !defined(READ_ONLY)
static int io_flash_write(io_entity_t *entity, const uintptr_t buffer,
		size_t length, size_t *length_written)
{
	if (map_mode==true)
	{
		ERROR("BootSPI is in MAP_MODE, cant write flash.");
		return -1;
	}

	assert(entity);
	assert(length_written);
	assert(buffer);
	unsigned long long pos_after;
	io_flash_file_state_t *fp = (io_flash_file_state_t *) entity->info;
	
	pos_after = fp->file_pos + length;
	assert((pos_after >= fp->file_pos) && (pos_after <= fp->size));
	io_flash_write(in_use_spi_id, (uint32_t)(fp->base + fp->file_pos - FLASH_MAP_BASE), (uint8_t*)buffer, (size_t)length);
	*length_written = length;
	fp->file_pos = pos_after;
	return 0;
}
#endif

static int io_flash_close(io_entity_t *entity)
{
	assert(entity);
	io_flash_file_state_t *fp = (io_flash_file_state_t *) entity->info;
	fp->in_use = 0;
	entity = NULL;
	return 0;
}

/* Exported functions */

/* Register the memmap driver with the IO abstraction */
int register_io_dev_flash(const io_dev_connector_t **dev_con)
{
	int result;
	assert(dev_con != NULL);

	result = io_register_device(&io_flash_dev_info);
	if (result == 0)
		*dev_con = &io_flash_dev_connector;

	return result;
}
