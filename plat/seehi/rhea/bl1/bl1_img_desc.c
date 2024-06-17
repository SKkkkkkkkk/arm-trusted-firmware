/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <common/bl_common.h>

image_desc_t bl1_image_descs[] = {
#if NS_BL1U_BASE
    {
	    .image_id = NS_BL1U_IMAGE_ID,
	    SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP,
		    VERSION_1, entry_point_info_t, NON_SECURE | EXECUTABLE),
	    .ep_info.pc = NS_BL1U_BASE,
    },
#endif
	    BL2_IMAGE_DESC,
    {
	    .image_id = INVALID_IMAGE_ID,
    }
};

/*******************************************************************************
 * This function does linear search for image_id and returns image_desc.
 ******************************************************************************/
image_desc_t *bl1_plat_get_image_desc(unsigned int image_id)
{
	unsigned int index = 0;

	while (bl1_image_descs[index].image_id != INVALID_IMAGE_ID) {
		if (bl1_image_descs[index].image_id == image_id)
			return &bl1_image_descs[index];
		index++;
	}

	return NULL;
}