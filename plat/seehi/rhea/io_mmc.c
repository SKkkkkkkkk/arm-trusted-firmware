#include <assert.h>
#include <common/debug.h>
#include <drivers/mmc.h>
#include <dw_mmc.h>
#include <io_mmc.h>
#include <platform_def.h>
#include <dw_apb_gpio.h>

#include "include/memmap.h"

typedef struct {
	// device
	int dev_idx;
	// block
	uintptr_t base;
	unsigned long long size;
	size_t block_size;
	unsigned long long file_pos;
	// status
	int inited;
	int in_use;
} io_mmc_state_t;

#define MAX_BLK_PER_TRANS 4

static io_mmc_state_t current_state = { .inited = 0 };
static unsigned long int dma_desc[32 * 2] __aligned(64) = { 0 };
static unsigned char _mmc_read_buf[MAX_BLK_PER_TRANS * 512] __aligned(16) = { 0 };

static int mmc_block_open(io_dev_info_t *dev_info __unused, const uintptr_t io_spec, io_entity_t *entity)
{
	assert(io_spec);
	assert(entity);

	if (current_state.in_use != 0) {
		WARN("mmc device is already active. Close first.\n");
		return -1;
	}

	io_mmc_io_open_spec_t *io_open_spec = (io_mmc_io_open_spec_t *)io_spec;

	current_state.base = io_open_spec->mmc_block.offset;
	current_state.size = io_open_spec->mmc_block.length;
	current_state.file_pos = 0;
	current_state.in_use = 1;
	current_state.block_size = io_open_spec->block_size;
	entity->info = (uintptr_t)&current_state;

	return 0;
}

static int mmc_block_seek(io_entity_t *entity, int mode, signed long long offset)
{
	assert(entity->info != (uintptr_t)NULL);

	io_mmc_state_t *cur = (io_mmc_state_t *)entity->info;

	assert((offset >= 0) && ((unsigned long long)offset < (cur->size)));

	switch (mode) {
	case IO_SEEK_SET:
		cur->file_pos = (unsigned long long)offset;
		break;
	case IO_SEEK_CUR:
		cur->file_pos += (unsigned long long)offset;
		break;
	default:
		return -EINVAL;
	}
	assert(cur->file_pos < cur->size);
	return 0;
}

static int mmc_block_read(io_entity_t *entity, uintptr_t buffer, size_t length, size_t *length_read)
{
	assert(entity->info != (uintptr_t)NULL);
	io_mmc_state_t *cur = (io_mmc_state_t *)entity->info;
	size_t blk_size = cur->block_size;
	unsigned long long start_blk_skip_byte = (cur->base + cur->file_pos) % blk_size;
	unsigned long long start_blk = (cur->base + cur->file_pos) / blk_size;
	unsigned long long blk_cnt = (start_blk_skip_byte + length + blk_size - 1) / blk_size;
	unsigned long long i = 0;
	unsigned long long readed = 0;
	unsigned long long cp_start = 0;
	unsigned long long cp_len = 0;
	size_t ret = 0;

	for (i = 0; i < blk_cnt; i = i + MAX_BLK_PER_TRANS) {
		ret = mmc_read_blocks(start_blk + i, (uintptr_t)_mmc_read_buf, blk_size * MAX_BLK_PER_TRANS);
		if (ret != blk_size * MAX_BLK_PER_TRANS) {
			WARN("mmc_read_blocks failed\n");
			return -1;
		}

		cp_start = (i != 0) ? 0 : start_blk_skip_byte;

		if (i == 0) {
			cp_len = blk_size * MAX_BLK_PER_TRANS - start_blk_skip_byte;
			cp_len = (cp_len > length) ? length : cp_len; // 当读取的内容只在一个blk里面, 只需要拷贝length
		} else if (i == (blk_cnt - 1)) {
			cp_len = length - readed; // 最后一次数据
		} else {
			cp_len = blk_size * MAX_BLK_PER_TRANS;
		}

		memcpy((void *)(buffer + readed), _mmc_read_buf + cp_start, cp_len);

		readed += cp_len;
	}

	cur->file_pos += readed;
	*length_read = readed;
	return 0;
}

static int mmc_block_close(io_entity_t *entity)
{
	assert(entity);
	io_mmc_state_t *fp = (io_mmc_state_t *)entity->info;
	fp->in_use = 0;

	entity = NULL;

	return 0;
}

static io_type_t mmc_dev_type(void)
{
	return IO_TYPE_MMC;
}

void mmc_iomux_set_default(void)
{
	pinmux_select(PORTA, 0, 7); // clk
	pinmux_select(PORTA, 1, 7); // cmd
	pinmux_select(PORTA, 2, 7); // d0
	pinmux_select(PORTA, 3, 7); // d1
	pinmux_select(PORTA, 4, 7); // d2
	pinmux_select(PORTA, 5, 7); // d3
	pinmux_select(PORTA, 6, 7); // d4
	pinmux_select(PORTA, 7, 7); // d5
	pinmux_select(PORTA, 8, 7); // d6
	pinmux_select(PORTA, 9, 7); // d7
	pinmux_select(PORTA, 25, 7); // rstn
}

static int mmc_dev_init(io_dev_info_t *dev_info, const uintptr_t init_params __unused)
{
	assert(dev_info);

	io_mmc_dev_spec_t *dev_spec = (io_mmc_dev_spec_t *)init_params;
	dw_mmc_params_t params;
	static struct mmc_device_info mmc_info;
	int i = 0;
	int j = 0;
	int ret = 0;
	unsigned char emmc_card_type[] = { MMC_IS_EMMC, MMC_IS_SD_HC };
	unsigned char emmc_bus_width[] = { MMC_BUS_WIDTH_8, MMC_BUS_WIDTH_4, MMC_BUS_WIDTH_1 };

	if (current_state.inited != 0) {
		return 0;
	}

	current_state.dev_idx = dev_spec->idx;

	if (dev_spec->idx == FIP_BACKEND_EMMC) {
		memset(&params, 0, sizeof(dw_mmc_params_t));
		params.reg_base = EMMC_BASE;
		params.desc_base = (unsigned long int)dma_desc;
		params.desc_size = sizeof(dma_desc);
		params.clk_rate = PLAT_MMC_CLK_IN_HZ;
		params.flags = 0;
		params.reg_base = EMMC_BASE;
		params.max_clk = 40000000;

		for (j = 0; j < sizeof(emmc_card_type) / sizeof(emmc_card_type[0]); j++) {
			for (i = 0; i < sizeof(emmc_bus_width) / sizeof(emmc_bus_width[0]); i++) {
				if ((emmc_bus_width[i] == MMC_BUS_WIDTH_8) &&
					(emmc_card_type[j] != MMC_IS_EMMC)) {
					continue;
				}
				mmc_iomux_set_default();

				pinmux_select(PORTA, 0, 0); // clk
				pinmux_select(PORTA, 1, 0); // cmd
				if (emmc_bus_width[i] == MMC_BUS_WIDTH_8) {
					pinmux_select(PORTA, 6, 0); // d4
					pinmux_select(PORTA, 7, 0); // d5
					pinmux_select(PORTA, 8, 0); // d6
					pinmux_select(PORTA, 9, 0); // d7
				}
				if ((emmc_bus_width[i] == MMC_BUS_WIDTH_4) ||
					(emmc_bus_width[i] == MMC_BUS_WIDTH_8)) {
					pinmux_select(PORTA, 3, 0); // d1
					pinmux_select(PORTA, 4, 0); // d2
					pinmux_select(PORTA, 5, 0); // d3
				}
				pinmux_select(PORTA, 2, 0); // d0
				pinmux_select(PORTA, 25, 0); // rstn

				params.bus_width = emmc_bus_width[i];
				mmc_info.mmc_dev_type = emmc_card_type[j];
				mmc_info.ocr_voltage = (mmc_info.mmc_dev_type == MMC_IS_EMMC) ?
							       OCR_VDD_MIN_1V7 : (OCR_3_3_3_4 | OCR_3_2_3_3);

				ret = dw_mmc_init(&params, &mmc_info);
				if (ret == 0) {
					current_state.inited = 1;
					return 0;
				}
			}
		}
	} else {
		WARN("unknown mmc device\n");
	}

	WARN("init mmc/sd failed!\n");
	return -1;
}

static int mmc_dev_close(io_dev_info_t *dev_info)
{
	dev_info->info = (uintptr_t)NULL;
	current_state.file_pos = 0;
	current_state.in_use = 1;
	return 0;
}

static const io_dev_funcs_t mmc_dev_funcs = {
	.type = mmc_dev_type,
	.open = mmc_block_open,
	.seek = mmc_block_seek,
	.read = mmc_block_read,
	.close = mmc_block_close,
	.dev_init = mmc_dev_init,
	.dev_close = mmc_dev_close,
};

static io_dev_info_t mmc_dev_info = { .funcs = &mmc_dev_funcs, .info = (uintptr_t)NULL };

static int mmc_dev_open(const uintptr_t dev_spec __unused, io_dev_info_t **dev_info)
{
	assert(dev_info != NULL);

	*dev_info = &mmc_dev_info;

	return 0;
}

static const io_dev_connector_t mmc_dev_connector = { .dev_open = mmc_dev_open };

int register_io_dev_mmc(const io_dev_connector_t **dev_con)
{
	int result;
	assert(dev_con != NULL);

	result = io_register_device(&mmc_dev_info);
	if (result == 0)
		*dev_con = &mmc_dev_connector;

	return result;
}
