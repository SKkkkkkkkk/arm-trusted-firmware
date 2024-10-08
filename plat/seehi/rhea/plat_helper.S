/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <platform_def.h>

	.globl	plat_my_core_pos
	.globl	plat_get_my_entrypoint
	.globl	platform_mem_init
	.globl	plat_rhea_calc_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush
	.globl  plat_secondary_cold_boot_setup
	.globl  plat_is_my_cpu_primary
	.globl	plat_report_exception

func plat_report_exception
	b .
endfunc plat_report_exception

func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_rhea_calc_core_pos
endfunc plat_my_core_pos

/*
 *  unsigned int plat_rhea_calc_core_pos(u_register_t mpidr);
 *  With this function: CorePos = (ClusterId * 4) + CoreId
 */
func plat_rhea_calc_core_pos
	and	x1, x0, #MPIDR_A55_CPU_MASK
	and	x0, x0, #MPIDR_A55_CLUSTER_MASK
	lsr x1, x1, 8
	lsr x0, x0, 8
	add	x0, x1, x0, LSR #(MPIDR_AFFINITY_BITS -\
				  PLATFORM_CPU_PER_CLUSTER_SHIFT)
	ret
endfunc plat_rhea_calc_core_pos

	/* -----------------------------------------------------
	 * unsigned int plat_is_my_cpu_primary (void);
	 *
	 * Find out whether the current cpu is the primary
	 * cpu.
	 * -----------------------------------------------------
	 */
func plat_is_my_cpu_primary
	mrs	x0, mpidr_el1
	and	x0, x0, #(MPIDR_A55_CLUSTER_MASK | MPIDR_A55_CPU_MASK)
	cmp	x0, #RHEA_PRIMARY_CPU
	cset	w0, eq
	ret
endfunc plat_is_my_cpu_primary

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset e.g
	 * mark the cpu's presence, mechanism to place it in a
	 * holding pen etc.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	/* Calculate address of our hold entry */
	bl	plat_my_core_pos
	lsl	x0, x0, #PLAT_RHEA_HOLD_ENTRY_SHIFT
	mov_imm	x2, PLAT_RHEA_HOLD_BASE

	/* Wait until we have a go */
poll_mailbox:
	ldr	x1, [x2, x0]
	cbz	x1, 1f

	/* Clear the mailbox again ready for next time. */
	mov x1, #PLAT_RHEA_HOLD_STATE_WAIT
	str x1, [x2, x0]

	/* Jump to the provided entrypoint. */
	mov_imm	x0, PLAT_RHEA_TRUSTED_MAILBOX_BASE
	ldr	x1, [x0]
	br	x1
1:
	wfe
	b	poll_mailbox
endfunc plat_secondary_cold_boot_setup

func plat_get_my_entrypoint
	// support only cold reset
	mov     x0, #0
	ret
endfunc plat_get_my_entrypoint

func platform_mem_init
	ret
endfunc platform_mem_init

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	ret
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	ret
endfunc plat_crash_console_putc

	/* ---------------------------------------------
	 * void plat_crash_console_flush(int c)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * Out : void.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func plat_crash_console_flush
	ret
endfunc plat_crash_console_flush
