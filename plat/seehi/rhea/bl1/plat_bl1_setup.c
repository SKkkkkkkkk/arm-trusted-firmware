/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <plat/common/platform.h>

#include <drivers/console.h>
#include <drivers/ti/uart/uart_16550.h>
#include <drivers/generic_delay_timer.h>

#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/mmio.h>
// #include <drivers/io/io_storage.h>
// #include <time_stamp.h>

/* Data structure which holds the extents of the trusted SRAM for BL1*/
static meminfo_t bl1_tzram_layout;
static meminfo_t bl2_tzram_layout;

static console_t console;

static bool is_fwu_needed = false;

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}

/*
 * Cannot use default weak implementation in plat_bl1_common.c.
 * Because the meminfo_t structure passing to BL2 via arg1 cannot be 
 * the default one which is at bl1_tzram_layout->total_base. An error
 * will happen if BL2 is just located at bl1_tzram_layout->total_base.
 */
int bl1_plat_handle_post_image_load(unsigned int image_id)
{
	meminfo_t *bl1_secram_layout;
	image_desc_t *image_desc;
	entry_point_info_t *ep_info;

	if (image_id != BL2_IMAGE_ID)
		return 0;

	/* Get the image descriptor */
	image_desc = bl1_plat_get_image_desc(BL2_IMAGE_ID);
	assert(image_desc != NULL);

	/* Get the entry point info */
	ep_info = &image_desc->ep_info;

	/* Find out how much free trusted ram remains after BL1 load */
	bl1_secram_layout = bl1_plat_sec_mem_layout();

	void bl1_calc_bl2_mem_layout(const meminfo_t *bl1_mem_layout,meminfo_t *bl2_mem_layout);
	bl1_calc_bl2_mem_layout(bl1_secram_layout, &bl2_tzram_layout);

	ep_info->args.arg1 = (uintptr_t)&bl2_tzram_layout;

	VERBOSE("BL1: BL2 memory layout address = %p\n",
		(void *)&bl2_tzram_layout);

	// TIME_STAMP(); // bl1_load_bl2 end.
	return 0;
}


/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void bl1_early_platform_setup(void)
{
	int ret;
	/* Initialize the console to provide early debug support */
	(void)console;

	mmio_write_32(GENERIC_TIMER_BASE + CNTCR_OFF,
		CNTCR_FCREQ(0U) | CNTCR_EN);

	ret = console_16550_register(PLAT_UART_BASE,
				 PLAT_UART_CLK_IN_HZ,
				 PLAT_CONSOLE_BAUDRATE,
				 &console);
	if(ret!=1)
		while(1);

	#define FRACTIONAL_VALUE_DELTA 625U
	uint8_t dlf_value;
	uint64_t fractional_value = ((PLAT_UART_CLK_IN_HZ*10000)/(16*PLAT_CONSOLE_BAUDRATE))%10000;
	dlf_value = fractional_value/FRACTIONAL_VALUE_DELTA;
	if((fractional_value%FRACTIONAL_VALUE_DELTA) >= (FRACTIONAL_VALUE_DELTA/2))
		++dlf_value;
	mmio_write_32(PLAT_UART_BASE + 0xc0, dlf_value);

	console_set_scope(&console, CONSOLE_FLAG_BOOT | CONSOLE_FLAG_RUNTIME);

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = BL_RAM_BASE;
	bl1_tzram_layout.total_size = BL_RAM_SIZE;

	generic_delay_timer_init();
	// TIME_STAMP_INIT();
}

/******************************************************************************
 * Perform the very early platform specific architecture setup.  This only
 * does basic initialization. Later architectural setup (bl1_arch_setup())
 * does not do anything platform specific.
 *****************************************************************************/
#define MAP_BL1_TOTAL MAP_REGION_FLAT( \
bl1_tzram_layout.total_base, \
bl1_tzram_layout.total_size, \
MT_MEMORY|MT_RW|MT_SECURE )

#define MAP_BL1_RO MAP_REGION_FLAT( \
BL_CODE_BASE, \
BL1_CODE_END-BL_CODE_BASE, \
MT_CODE|MT_SECURE)

#define MAP_DEVICE	MAP_REGION_FLAT( \
DEVICE_START_BASE, \
(DEVICE_END_BASE - DEVICE_START_BASE), \
MT_DEVICE|MT_RW|MT_SECURE)


#define MAP_SHARED_RAM	MAP_REGION_FLAT( \
SHARED_RAM_BASE,			\
SHARED_RAM_SIZE,			\
MT_DEVICE|MT_RW|MT_SECURE)

void bl1_plat_arch_setup(void)
{
#if (BL1_BL2_MMU_SWITCH == 1)
	const mmap_region_t plat_bl1_region[] = {
		MAP_BL1_TOTAL,
		MAP_BL1_RO,
		MAP_DEVICE,
		MAP_SHARED_RAM,
		{0}
	};
	mmap_add(plat_bl1_region);
	init_xlat_tables();
	enable_mmu_el3(0);
#endif
}

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}

void bl1_platform_setup(void)
{
	void plat_io_setup(void);
	plat_io_setup();
	
	bool plat_bl1_fwu_needed(void);
	is_fwu_needed = plat_bl1_fwu_needed();
}

/*******************************************************************************
 * The following function checks if Firmware update is needed,
 * by checking if TOC in FIP image is valid or not.
 ******************************************************************************/
unsigned int bl1_plat_get_next_image_id(void)
{
	return  is_fwu_needed ? NS_BL1U_IMAGE_ID : BL2_IMAGE_ID;
}

// int bl1_plat_handle_pre_image_load(unsigned int image_id __unused)
// {
// 	TIME_STAMP(); // bl1_load_bl2 start.
// 	return 0;
// }