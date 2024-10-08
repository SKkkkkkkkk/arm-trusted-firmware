/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/bl_common.h>

#include <plat/common/platform.h>
#include <platform_def.h>
#include <plat_common.h>
#include "./rhea_gicv3.h"

#include <drivers/generic_delay_timer.h>

#include <lib/xlat_tables/xlat_tables_v2.h>

#include <time_stamp.h>

/*
 * Placeholder variables for copying the arguments that have been passed to
 * BL3-1 from BL2.
 */
static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

/*******************************************************************************
 * Perform any BL3-1 early platform setup.  Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & EL3 in BL1) before
 * they are lost (potentially). This needs to be done before the MMU is
 * initialized so that the memory layout can be used while creating page
 * tables. BL2 has flushed this information to memory, so we are guaranteed
 * to pick up good data.
 ******************************************************************************/
void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	TIME_STAMP();
	/* Initialize the console to provide early debug support */
	console_16550_with_dlf_init();

	/*
	 * Check params passed from BL2
	 */
	bl_params_t *params_from_bl2 = (bl_params_t *)arg0;

	assert(params_from_bl2);
	assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
	assert(params_from_bl2->h.version >= VERSION_2);

	bl_params_node_t *bl_params = params_from_bl2->head;

	/*
	 * Copy BL33 and BL32 (if present), entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	while (bl_params) {
		if (bl_params->image_id == BL32_IMAGE_ID)
			bl32_image_ep_info = *bl_params->ep_info;

		if (bl_params->image_id == BL33_IMAGE_ID)
			bl33_image_ep_info = *bl_params->ep_info;

		bl_params = bl_params->next_params_info;
	}

	if (!bl33_image_ep_info.pc)
		panic();
	
	generic_delay_timer_init();
}


#define MAP_BL31_TOTAL MAP_REGION_FLAT( \
BL31_BASE, \
(BL31_END - BL31_BASE), \
MT_MEMORY|MT_RW|MT_SECURE )

#define MAP_BL31_RO MAP_REGION_FLAT( \
BL_CODE_BASE, \
BL_CODE_END-BL_CODE_BASE, \
MT_CODE|MT_SECURE)

void bl31_plat_arch_setup(void)
{
	const mmap_region_t plat_bl1_region[] = {
		MAP_BL31_TOTAL,
		MAP_BL31_RO,
		MAP_DEVICE,
		MAP_SHARED_RAM,
		{0}
	};
	mmap_add(plat_bl1_region);
	init_xlat_tables();
	enable_mmu_el3(0);
}

void bl31_platform_setup(void)
{
	plat_rhea_gic_init();
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image
 * for the security state specified. BL3-3 corresponds to the non-secure
 * image type while BL3-2 corresponds to the secure image type. A NULL
 * pointer is returned if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	next_image_info = (type == NON_SECURE)
			? &bl33_image_ep_info : &bl32_image_ep_info;
	/*
	 * None of the images on the ARM development platforms can have 0x0
	 * as the entrypoint
	 */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}
