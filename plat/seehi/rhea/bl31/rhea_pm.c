/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <arch_helpers.h>

#include <common/debug.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include "./rhea_gicv3.h"

#include <lib/psci/psci.h>

#define Todo() panic()
/*
 * The secure entry point to be used on warm reset.
 */
static unsigned long secure_entrypoint;
static unsigned int level = 0;

/* Make composite power state parameter till power level 0 */
#if PSCI_EXTENDED_STATE_ID

#define rhea_make_pwrstate_lvl0(lvl0_state, pwr_lvl, type) \
		(((lvl0_state) << PSTATE_ID_SHIFT) | \
		 ((type) << PSTATE_TYPE_SHIFT))
#else
#define rhea_make_pwrstate_lvl0(lvl0_state, pwr_lvl, type) \
		(((lvl0_state) << PSTATE_ID_SHIFT) | \
		 ((pwr_lvl) << PSTATE_PWR_LVL_SHIFT) | \
		 ((type) << PSTATE_TYPE_SHIFT))
#endif /* PSCI_EXTENDED_STATE_ID */


#define rhea_make_pwrstate_lvl1(lvl1_state, lvl0_state, pwr_lvl, type) \
		(((lvl1_state) << PLAT_LOCAL_PSTATE_WIDTH) | \
		 rhea_make_pwrstate_lvl0(lvl0_state, pwr_lvl, type))

/*
 *  The table storing the valid idle power states. Ensure that the
 *  array entries are populated in ascending order of state-id to
 *  enable us to use binary search during power state validation.
 *  The table must be terminated by a NULL entry.
 */
static const unsigned int rhea_pm_idle_states[] = {
	/* State-id - 0x01 */
	rhea_make_pwrstate_lvl1(PLAT_LOCAL_STATE_RUN, PLAT_LOCAL_STATE_RET,
				MPIDR_AFFLVL0, PSTATE_TYPE_STANDBY),
	/* State-id - 0x02 */
	rhea_make_pwrstate_lvl1(PLAT_LOCAL_STATE_RUN, PLAT_LOCAL_STATE_OFF,
				MPIDR_AFFLVL0, PSTATE_TYPE_POWERDOWN),
	/* State-id - 0x22 */
	rhea_make_pwrstate_lvl1(PLAT_LOCAL_STATE_OFF, PLAT_LOCAL_STATE_OFF,
				MPIDR_AFFLVL1, PSTATE_TYPE_POWERDOWN),
	0,
};

/*******************************************************************************
 * Platform handler called to check the validity of the power state
 * parameter. The power state parameter has to be a composite power state.
 ******************************************************************************/
static int rhea_validate_power_state(unsigned int power_state,
				psci_power_state_t *req_state)
{
	unsigned int state_id;
	int i;

	assert(req_state);

	/*
	 *  Currently we are using a linear search for finding the matching
	 *  entry in the idle power state array. This can be made a binary
	 *  search if the number of entries justify the additional complexity.
	 */
	for (i = 0; !!rhea_pm_idle_states[i]; i++) {
		if (power_state == rhea_pm_idle_states[i])
			break;
	}

	/* Return error if entry not found in the idle state array */
	if (!rhea_pm_idle_states[i])
		return PSCI_E_INVALID_PARAMS;

	i = 0;
	state_id = psci_get_pstate_id(power_state);
	if(state_id == 0x22){
		level=1;
	}else{
		level=0;
	}

	/* Parse the State ID and populate the state info parameter */
	while (state_id) {
		req_state->pwr_domain_state[i++] = state_id &
						PLAT_LOCAL_PSTATE_MASK;
		state_id >>= PLAT_LOCAL_PSTATE_WIDTH;
	}

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Platform handler called to check the validity of the non secure
 * entrypoint.
 ******************************************************************************/
static int rhea_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	return ((entrypoint >= (uintptr_t)DDR_BASE) \
		&& (entrypoint < ((uintptr_t)DDR_BASE + DDR_SIZE))) ? PSCI_E_SUCCESS : PSCI_E_INVALID_ADDRESS;
}

void rhea_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;

	for (i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

/*******************************************************************************
 * Platform handler called when a CPU is about to enter standby.
 ******************************************************************************/
static void rhea_cpu_standby(plat_local_state_t cpu_state)
{
	u_register_t scr = read_scr_el3();
	assert(cpu_state == PLAT_LOCAL_STATE_RET);

	/*
	 * Enable the Non-secure interrupt to wake the CPU.
	 * In GICv3 affinity routing mode, the Non-secure Group 1 interrupts
	 * use Physical FIQ at EL3 whereas in GICv2, Physical IRQ is used.
	 * Enabling both the bits works for both GICv2 mode and GICv3 affinity
	 * routing mode.
	 */
	write_scr_el3(scr | SCR_IRQ_BIT | SCR_FIQ_BIT);
	isb();

	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
	dsb();
	wfi();

	/*
	 * Restore SCR_EL3 to the original value, synchronisation of SCR_EL3
	 * is done by eret in el3_exit() to save some execution cycles.
	 */
	write_scr_el3(scr);
}

/*******************************************************************************
 * Platform handler called when a power domain is about to be turned on. The
 * mpidr determines the CPU to be turned on.
 ******************************************************************************/
static int rhea_pwr_domain_on(u_register_t mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned pos = plat_core_pos_by_mpidr(mpidr);
	assert(pos < PLATFORM_CORE_COUNT);
	uint64_t *hold_base = (uint64_t *)PLAT_RHEA_HOLD_BASE;

	hold_base[pos] = PLAT_RHEA_HOLD_STATE_GO;
	dsbsy();
	sev();
	return rc;
}

/*******************************************************************************
 * Platform handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
static void rhea_pwr_domain_off(const psci_power_state_t *target_state)
{
	rhea_pwr_gic_off();
}

void __dead2 plat_secondary_cold_boot_setup(void);

static void __dead2
rhea_pwr_domain_pwr_down_wfi(const psci_power_state_t *target_state)
{
	disable_mmu_el3();
	plat_secondary_cold_boot_setup();
}

/*******************************************************************************
 * Platform handler called when a power domain is about to be suspended. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void rhea_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	ERROR("CPU Suspend With Power Down: operation not supported.\n");
	panic();
}

/*******************************************************************************
 * Platform handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
void rhea_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	assert(target_state->pwr_domain_state[MPIDR_AFFLVL0] ==
					PLAT_LOCAL_STATE_OFF);

	rhea_pwr_gic_on_finish();
}

/*******************************************************************************
 * Platform handler called when a power domain has just been powered on after
 * having been suspended earlier. The target_state encodes the low power state
 * that each level has woken up from.
 ******************************************************************************/
void rhea_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	ERROR("No Fking way u could just wakeup from here.\n");
	panic();
}

/*******************************************************************************
 * Platform handlers to shutdown/reboot the system
 ******************************************************************************/
static void __dead2 rhea_system_off(void)
{
	ERROR("System Off: operation not supported.\n");
	panic();
}

static void __dead2 rhea_system_reset(void)
{
	ERROR("System Reset: operation not supported.\n");
	panic();
}

static const plat_psci_ops_t plat_rhea_psci_pm_ops = {
	.cpu_standby = rhea_cpu_standby,
	.pwr_domain_on = rhea_pwr_domain_on,
	.pwr_domain_off = rhea_pwr_domain_off,
	.pwr_domain_pwr_down_wfi = rhea_pwr_domain_pwr_down_wfi,
	.pwr_domain_suspend = rhea_pwr_domain_suspend,
	.pwr_domain_on_finish = rhea_pwr_domain_on_finish,
	.pwr_domain_suspend_finish = rhea_pwr_domain_suspend_finish,
	.system_off = rhea_system_off,
	.system_reset = rhea_system_reset,
	.validate_power_state = rhea_validate_power_state,
	.validate_ns_entrypoint = rhea_validate_ns_entrypoint,
	.get_sys_suspend_power_state = rhea_get_sys_suspend_power_state
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	uintptr_t *mailbox = (void *) PLAT_RHEA_TRUSTED_MAILBOX_BASE;

	*mailbox = sec_entrypoint;
	secure_entrypoint = (unsigned long) sec_entrypoint;
	*psci_ops = &plat_rhea_psci_pm_ops;

	return 0;
}