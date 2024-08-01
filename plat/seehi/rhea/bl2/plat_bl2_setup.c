/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <plat/common/platform.h>
#include <plat_common.h>
#include <platform_def.h>
#include <chip.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>

#include <drivers/console.h>
#include <drivers/ti/uart/uart_16550.h>
#include <drivers/generic_delay_timer.h>
#include <pcie.h>

#include <lib/utils.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <time_stamp.h>
#include "dram.h"

#ifdef SEEHI_SECUREBOOT
#include <seehi_secureboot.h>
#endif

/* Data structure which holds the extents of the trusted SRAM for BL2 */
static meminfo_t bl2_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

void bl2_early_platform_setup2(u_register_t arg0, u_register_t arg1,
			       u_register_t arg2, u_register_t arg3)
{
	int ret = 0;

	meminfo_t *mem_layout = (void *)arg1;

	if (IS_ASIC == 1)
		set_clk();

	/* Initialize the console to provide early debug support */
	console_16550_with_dlf_init();

	ret = rhea_pcie_ep_init(); //need bl2 first init
	if(ret != 0)
		ERROR("BL2 rhea_pcie_ep_init failed\n");

	/* Setup the BL2 memory layout */
	bl2_tzram_layout = *mem_layout;

	dram_init();

	generic_delay_timer_init();
}

void bl2_platform_setup(void)
{

}

#define MAP_BL2_TOTAL MAP_REGION_FLAT( \
bl2_tzram_layout.total_base, \
bl2_tzram_layout.total_size, \
MT_MEMORY|MT_RW|MT_SECURE )

#define MAP_BL2_RO MAP_REGION_FLAT( \
BL_CODE_BASE, \
BL_CODE_END-BL_CODE_BASE, \
MT_CODE|MT_SECURE)

#define MAP_BL33 MAP_REGION_FLAT( \
BL33_IMAGE_OFFSET,			\
BL33_IMAGE_MAX_SIZE,			\
MT_MEMORY|MT_RW|MT_NS)

#ifdef NEED_BL31_1
#define MAP_BL31_1 MAP_REGION_FLAT( \
BL31_1_IMAGE_BASE, \
BL31_1_IMAGE_LIMIT-BL31_1_IMAGE_BASE, \
MT_NON_CACHEABLE|MT_RW|MT_SECURE)
#endif

void bl2_plat_arch_setup(void)
{
	const mmap_region_t plat_bl2_region[] = {
		MAP_BL2_TOTAL,
		MAP_BL2_RO,
		MAP_BL33,
		MAP_DEVICE,
		MAP_SHARED_RAM,
	#ifdef NEED_BL31_1
		MAP_BL31_1,
	#endif
		{0}
	};
	mmap_add(plat_bl2_region);
	init_xlat_tables();
	enable_mmu_el1(0);

	// plat_io_setup() after mmu enabled is faster.
	plat_io_setup();
}

/*******************************************************************************
 * Gets SPSR for BL33 entry
 ******************************************************************************/
static uint32_t rhea_get_spsr_for_bl33_entry(void)
{
	uint32_t spsr;
	unsigned int mode;

	/* Figure out what mode we enter the non-secure world in */
	mode = (el_implemented(2) != EL_IMPL_NONE) ? MODE_EL2 : MODE_EL1;

	/*
	 * TODO: Consider the possibility of specifying the SPSR in
	 * the FIP ToC and allowing the platform to have a say as
	 * well.
	 */
	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	return spsr;
}

static int rhea_bl2_handle_post_image_load(unsigned int image_id)
{
	int err = 0;
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);
	assert(bl_mem_params);

	switch (image_id) {
#ifdef NEED_BL31_1
	case BL31_1_IMAGE_ID:
		TIME_STAMP();
		uint32_t bl31_1_base = (uint32_t)bl_mem_params->image_info.image_base;
		ERROR("BL2: This routine is a workaround for dram problem.\n");
		ERROR("BL2: About to wake up Core3 to run BL31_1.\n");
		ERROR("BL2: BL31_1_BASE = 0x%x\n", bl31_1_base);
		*(volatile uint64_t*)PLAT_RHEA_TRUSTED_MAILBOX_BASE = bl31_1_base;
		uint64_t *hold_base = (uint64_t *)PLAT_RHEA_HOLD_BASE;
		hold_base[3] = PLAT_RHEA_HOLD_STATE_GO;
		dsbsy();
		sev();
		while(1) asm volatile("wfe");
		break;
#endif
	case BL31_IMAGE_ID:
	#ifdef SEEHI_SECUREBOOT
		/* Verify the signature of BL31 image */
		if(!seehi_secureboot_verify(BL31_IMAGE_ID, &bl_mem_params->image_info)) {
			ERROR("BL31 image verification failed\n");
			err = -1;
		}
	#endif
		TIME_STAMP();
		break;
	case BL33_IMAGE_ID:
		TIME_STAMP();
#if ARM_LINUX_KERNEL_AS_BL33
		/*
		 * According to the file ``Documentation/arm64/booting.txt`` of
		 * the Linux kernel tree, Linux expects the physical address of
		 * the device tree blob (DTB) in x0, while x1-x3 are reserved
		 * for future use and must be 0.
		 */
		bl_mem_params->ep_info.args.arg0 =
			(u_register_t)ARM_PRELOADED_DTB_BASE;
		bl_mem_params->ep_info.args.arg1 = 0U;
		bl_mem_params->ep_info.args.arg2 = 0U;
		bl_mem_params->ep_info.args.arg3 = 0U;
#else
		/* BL33 expects to receive the primary CPU MPID (through x0) */
		bl_mem_params->ep_info.args.arg0 = (MPIDR_A55_CPU_MASK & read_mpidr()) >> MPIDR_AFF1_SHIFT;
#endif

		bl_mem_params->ep_info.spsr = rhea_get_spsr_for_bl33_entry();
		break;
	default:
		/* Do nothing in default case */
		break;
	}

	return err;
}

/*******************************************************************************
 * This function can be used by the platforms to update/use image
 * information for given `image_id`.
 ******************************************************************************/
int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	return rhea_bl2_handle_post_image_load(image_id);
}

int bl2_plat_handle_pre_image_load(unsigned int image_id __unused)
{
	TIME_STAMP();
	return 0;
}
