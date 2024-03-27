#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <platform_def.h>
#include <common/debug.h>
#include <tools_share/firmware_image_package.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_storage.h>
#include <dw_apb_gpio.h>

enum boot_device {
	BOOT_DEVICE_BOOTSPI,
	BOOT_DEVICE_EMMC,
	BOOT_DEVICE_MEMMAP,
	BOOT_DEVICE_NONE
};

static enum boot_device boot_dev = BOOT_DEVICE_NONE;

/* IO devices */
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;

static const io_dev_connector_t *backend_dev_con;
static uintptr_t backend_dev_handle;


static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

#ifdef NEED_SCP_BL2
static const io_uuid_spec_t scp_bl2_uuid_spec = {
	.uuid = UUID_SCP_FIRMWARE_SCP_BL2,
};
#endif

static int check_fip(const uintptr_t dev_init_spec, const uintptr_t io_open_spec);
static int check_backend(const uintptr_t dev_init_spec, const uintptr_t io_open_spec);

struct plat_io_policy {
	uintptr_t *dev_handle;
	// uintptr_t dev_open_spec;  // never used.
	uintptr_t dev_init_spec;
	uintptr_t io_open_spec;
	int (*check)(const uintptr_t dev_init_spec, const uintptr_t io_open_spec);
};

static struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		.dev_handle = &backend_dev_handle,
		.dev_init_spec = (uintptr_t)NULL,
		.io_open_spec = (uintptr_t)NULL,
		.check = check_backend
	},
	[BL2_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.dev_init_spec = (uintptr_t)FIP_IMAGE_ID,
		.io_open_spec = (uintptr_t)&bl2_uuid_spec,
		.check = check_fip
	},
	[BL31_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.dev_init_spec = (uintptr_t)FIP_IMAGE_ID,
		.io_open_spec = (uintptr_t)&bl31_uuid_spec,
		.check = check_fip
	},
	[BL33_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.dev_init_spec = (uintptr_t)FIP_IMAGE_ID,
		.io_open_spec = (uintptr_t)&bl33_uuid_spec,
		.check = check_fip
	},
#ifdef NEED_SCP_BL2
	[SCP_BL2_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.dev_init_spec = (uintptr_t)FIP_IMAGE_ID,
		.io_open_spec = (uintptr_t)&scp_bl2_uuid_spec,
		.check = check_fip
	},
#endif
};

enum boot_device get_boot_dev(void)
{
	pinmux_select(PORTB, FIP_STORAGE_KEY_PIN0, 7);
	pinmux_select(PORTB, FIP_STORAGE_KEY_PIN1, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTB,
		.pin = FIP_STORAGE_KEY_PIN0,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode
	};
	gpio_init(&gpio_init_config);
	gpio_init_config.pin = FIP_STORAGE_KEY_PIN1;
	gpio_init(&gpio_init_config);

	uint8_t pin_status = \
	(uint8_t)gpio_read_pin(PORTB, FIP_STORAGE_KEY_PIN1) << 1 | 
	(uint8_t)gpio_read_pin(PORTB, FIP_STORAGE_KEY_PIN0);

	if(pin_status < (uint8_t)BOOT_DEVICE_MEMMAP) // GPIO cannot use BOOT_DEVICE_MEMMAP.
		return (enum boot_device)pin_status;
	return BOOT_DEVICE_NONE;
}

int io_fip_setup(void)
{
	int io_result;

	io_result = register_io_dev_fip(&fip_dev_con);
	assert(io_result == 0);

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, (uintptr_t)NULL,
				&fip_dev_handle);
	assert(io_result == 0);

	return io_result;
}

static int memmap_io_setup(void* arg __unused)
{
	int io_result;

	io_result = register_io_dev_memmap(&backend_dev_con);
	assert(io_result == 0);

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(backend_dev_con, (uintptr_t)NULL,
				&backend_dev_handle);
	assert(io_result == 0);

	static const io_block_spec_t fip_block_spec = {
		.offset = MEMMAP_FIP_BASE,
		.length = MEMMAP_FIP_SIZE
	};
	policies[FIP_IMAGE_ID].dev_init_spec = (uintptr_t)NULL;
	policies[FIP_IMAGE_ID].io_open_spec = (uintptr_t)&fip_block_spec;

	io_result = mmap_add_dynamic_region(fip_block_spec.offset, fip_block_spec.offset,
		fip_block_spec.length, MT_MEMORY | MT_RW | MT_NS);
	if (io_result != 0) {
		ERROR("Error while mapping MEMMAP_FIP (%d).\n", io_result);
		panic();
	}
	return io_result;
}

static int (* const io_setup_table[])(void*) = {
	[BOOT_DEVICE_BOOTSPI]	= memmap_io_setup,
	[BOOT_DEVICE_EMMC]		= memmap_io_setup,
	[BOOT_DEVICE_MEMMAP]	= memmap_io_setup,
};

int plat_io_setup(void)
{
	int io_result;

	boot_dev = get_boot_dev();
	if (boot_dev == BOOT_DEVICE_NONE) {
		ERROR("Boot Device detection failed, Check GPIO_PROGSEL\n");
		while(1) asm volatile("wfi");
	}

	io_result = io_setup_table[boot_dev]((void*)boot_dev);
	assert(io_result==0);

	io_result = io_fip_setup();
	assert(io_result==0);

	/* Ignore improbable errors in release builds */
	return io_result;
}

static int check_fip(const uintptr_t dev_init_spec, const uintptr_t io_open_spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, dev_init_spec);
	if (result == 0 && dev_init_spec != (uintptr_t)NULL \
					&& io_open_spec  != (uintptr_t)NULL) 
	{
		result = io_open(fip_dev_handle, io_open_spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

static int check_backend(const uintptr_t dev_init_spec, const uintptr_t io_open_spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(backend_dev_handle, dev_init_spec);
	if (result == 0 && io_open_spec  != (uintptr_t)NULL) {
		result = io_open(backend_dev_handle, io_open_spec, &local_image_handle);
		if (result == 0) {
			io_close(local_image_handle);
		}
	}

	return result;
}

int plat_get_image_source(unsigned int image_id,
			uintptr_t *dev_handle,
			uintptr_t *image_spec)
{
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->dev_init_spec, policy->io_open_spec);
	if (result == 0) {
		*image_spec = policy->io_open_spec;
		*dev_handle = *(policy->dev_handle);
	} else {
		// VERBOSE("Trying alternative IO\n");
		// result = get_alt_image_source(image_id, dev_handle, image_spec);
		ERROR("plat_get_image_source error!\n");
	}
	return result;
}

/*
 * See if a Firmware Image Package is available,
 * by checking if TOC is valid or not.
 */
bool plat_bl1_fwu_needed(void)
{
	assert(fip_dev_handle);

	pinmux_select(PORTB, FWU_KEY_PIN, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTB,
		.pin = FWU_KEY_PIN,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode
	};
	gpio_init(&gpio_init_config);
	if(gpio_read_pin(PORTB, FWU_KEY_PIN) == 0)
	{
		INFO("FWU is about to be executed because of the FWU_KEY is pressed.\n\r");
		return true;
	}

	if(io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID) != 0)
	{
		ERROR("FWU is about to be executed because of the FIP is not found.\n\r");
		return true;
	}

	return false;
}