/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <drivers/arm/fvp/fvp_pwrc.h>
#include <plat/arm/common/arm_config.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>

/* The FVP VE power domain tree descriptor */
static const unsigned char power_domain_tree_desc[] = {
	PLATFORM_CLUSTER_COUNT,
	PLATFORM_CORE_COUNT
};

/*******************************************************************************
 * This function returns the topology according to FVP_VE_CLUSTER_COUNT.
 ******************************************************************************/
const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return power_domain_tree_desc;
}

/*******************************************************************************
 * Currently FVP VE has only been tested with one core, therefore 0 is returned.
 ******************************************************************************/
int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	if (mpidr & MPIDR_A55_CLUSTER_MASK) {
		return -1;
	}

	if (((mpidr & MPIDR_A55_CPU_MASK) >> MPIDR_AFF1_SHIFT) >= PLATFORM_CORE_COUNT) {
		return -1;
	}

	unsigned int plat_rhea_calc_core_pos(u_register_t mpidr);
	return plat_rhea_calc_core_pos(mpidr);
}
