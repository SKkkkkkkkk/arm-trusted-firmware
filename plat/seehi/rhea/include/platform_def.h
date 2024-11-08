/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <arch.h>
#include <lib/utils_def.h>
#include <common/tbbr/tbbr_img_def.h>
#include <plat/common/common_def.h>
#include <memmap.h>
#ifndef __ASSEMBLER__
#	include <cru.h>
#endif

#define PLAT_XLAT_TABLES_DYNAMIC 1

#ifdef FIRMWARE_WELCOME_STR
#	undef FIRMWARE_WELCOME_STR
#	define FIRMWARE_WELCOME_STR "Booting Rhea Trusted Firmware\n"
#endif

#if ARM_ARCH_MAJOR == 7
#	define PLATFORM_MAX_CPUS_PER_CLUSTER	U(4)
#	define PLATFORM_CLUSTER_COUNT			U(1)
#	define PLATFORM_CLUSTER0_CORE_COUNT		PLATFORM_MAX_CPUS_PER_CLUSTER
#	define PLATFORM_CLUSTER1_CORE_COUNT		U(0)
#else
#	define PLATFORM_MAX_CPUS_PER_CLUSTER	U(4)
#endif
/*
 * Define the number of cores per cluster used in calculating core position.
 * The cluster number is shifted by this value and added to the core ID,
 * so its value represents log2(cores/cluster).
 * Default is 2**(2) = 4 cores per cluster.
 */
#define PLATFORM_CPU_PER_CLUSTER_SHIFT	U(2)

#define MPIDR_A55_CPU_MASK 		(MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT)
#define MPIDR_A55_CLUSTER_MASK	(MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT)

#define PLATFORM_CLUSTER_COUNT		U(1)
#ifdef NEED_BL31_1
#	define PLATFORM_CORE_COUNT			U(3)
#else
#	define PLATFORM_CORE_COUNT			U(4)
#endif

#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CLUSTER_COUNT + PLATFORM_CORE_COUNT)
#define PLAT_MAX_PWR_LVL		U(1)
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(2)

/* Local power state for power domains in Run state. */
#define PLAT_LOCAL_STATE_RUN		U(0)
/* Local power state for retention. Valid only for CPU power domains */
#define PLAT_LOCAL_STATE_RET		U(1)
/*
 * Local power state for OFF/power-down. Valid for CPU and cluster power
 * domains.
 */
#define PLAT_LOCAL_STATE_OFF		2

/*
 * Macros used to parse state information from State-ID if it is using the
 * recommended encoding for State-ID.
 */
#define PLAT_LOCAL_PSTATE_WIDTH		4
#define PLAT_LOCAL_PSTATE_MASK		((1 << PLAT_LOCAL_PSTATE_WIDTH) - 1)

/*
 * Some data must be aligned on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 */
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

#define MAX_IO_DEVICES			4
#define MAX_IO_HANDLES			4
#define MAX_IO_BLOCK_DEVICES	1

#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)
#if defined(IMAGE_BL1)
#	define MAX_MMAP_REGIONS		5
#	define MAX_XLAT_TABLES		5
#elif defined(IMAGE_BL2)
#	define MAX_MMAP_REGIONS		6
#	define MAX_XLAT_TABLES		6
#elif defined(IMAGE_BL31)
#	define MAX_MMAP_REGIONS		5
#	define MAX_XLAT_TABLES		7
#endif

/***FIP Storages***/
#define FIP_BACKEND_FLASH	0
#define FIP_BACKEND_EMMC	1
#define FIP_BACKEND_MEMMAP	2

#define FWU_KEY_PIN 1
#define BYPSECURE_KEY_PIN 	 2
#define FIP_STORAGE_KEY_PIN0 4

/*
 * Partition memory into secure ROM, non-secure DRAM, secure "SRAM",
 * and secure DRAM.
 */
#define ROM_BASE A55_BOOTROM_BASE
#define ROM_SIZE UL(0x00040000) //256KiB

#define SRAM_BASE APRAM_BASE
#define SRAM_SIZE UL(0x00080000) //512KiB

#define DDR_BASE AP_DRAM_BASE
#define DDR_SIZE UL(0x18000000) //384MiB

#define FLASH_MAP_BASE BOOTFLASH_BASE
#define FLASH_MAP_SIZE UL(0x01000000) //24bit, 16MiB

#define DEVICE_START_BASE				UART0_BASE
#define DEVICE_END_BASE					round_up(TCM_CFG_BASE + 16*1024*1024U, PAGE_SIZE)

#define PLATFORM_STACK_SIZE 	U(4096)
#define SHARED_RAM_BASE			SRAM_BASE 

#define RHEA_PRIMARY_CPU				 U(0)
#define PLAT_RHEA_TRUSTED_MAILBOX_BASE	 SHARED_RAM_BASE
#define PLAT_RHEA_TRUSTED_MAILBOX_SIZE	 (8 + PLAT_RHEA_HOLD_SIZE)
#define PLAT_RHEA_HOLD_ENTRY_SHIFT		 3
#define PLAT_RHEA_HOLD_ENTRY_SIZE		 (1 << PLAT_RHEA_HOLD_ENTRY_SHIFT)
#define PLAT_RHEA_HOLD_BASE			 	 (PLAT_RHEA_TRUSTED_MAILBOX_BASE + 8)
#define PLAT_RHEA_HOLD_SIZE			 	 (PLATFORM_CORE_COUNT * PLAT_RHEA_HOLD_ENTRY_SIZE)
#define PLAT_RHEA_HOLD_STATE_WAIT		 0
#define PLAT_RHEA_HOLD_STATE_GO		 	 1

// TF_A_TOTAL_RAM = BL_RAM_SIZE+SHARED_RAM_SIZE = 512KiB
#define TF_A_TOTAL_RAM_SIZE		(SRAM_SIZE)
#define SHARED_RAM_SIZE			U(0x1000) // 4KiB
#define RESERVED_RAM_BASE		(SHARED_RAM_BASE + SHARED_RAM_SIZE)
#define RESERVED_RAM_SIZE		U(0x1F000) // 124KiB
#define BL_RAM_SIZE				(TF_A_TOTAL_RAM_SIZE - SHARED_RAM_SIZE - RESERVED_RAM_SIZE)

#define BL_RAM_BASE			(SHARED_RAM_BASE + SHARED_RAM_SIZE + RESERVED_RAM_SIZE)
#define BL_RAM_LIMIT		(BL_RAM_BASE + BL_RAM_SIZE)

/*
 * BL1 specific defines.
 *
 * BL1 RW data is relocated from ROM to RAM at runtime so we need 2 sets of
 * addresses.
 * Put BL1 RW at the top of the Secure SRAM. BL1_RW_BASE is calculated using
 * the current BL1 RW debug size plus a little space for growth.
 */
#define BL1_RO_BASE			ROM_BASE
#define BL1_RO_LIMIT		(ROM_BASE + (76*1024))
#define NS_BL1U_BASE 		BL1_RO_LIMIT  // NS_BL1U


#define BL1_RW_BASE			(BL1_RW_LIMIT - (32*1024) - (32*1024))
#define BL1_RW_LIMIT		(BL_RAM_LIMIT)

/*
 * BL3-1 specific defines.
 *
 * Put BL3-1 at the top of the Trusted SRAM. BL31_BASE is calculated using the
 * current BL3-1 debug size plus a little space for growth.
 */
#define BL31_BASE				(BL31_PROGBITS_LIMIT - (52*1024) - (32*1024))
#define BL31_PROGBITS_LIMIT		(BL1_RW_BASE)
#define BL31_LIMIT				(BL_RAM_LIMIT)

/*
 * BL2 specific defines.
 *
 * Put BL2 just below BL3-1. BL2_BASE is calculated using the current BL2 debug
 * size plus a little space for growth.
 */
#define BL2_BASE			(BL31_BASE - (140*1024) - (96*1024))
#define BL2_LIMIT			(BL31_BASE)

#ifdef NEED_SCP_BL2
#	define SCP_BL2_BASE				(0x79600000)
#	define SCP_BL2_LIMIT			(_1MiB + SCP_BL2_BASE)
#endif

/* Special value used to verify platform parameters from BL2 to BL31 */
#define RHEA_BL31_PLAT_PARAM_VAL	0x0f1e2d3c4b5a6978ULL

#define MAP_DEVICE	MAP_REGION_FLAT( \
DEVICE_START_BASE, \
(DEVICE_END_BASE - DEVICE_START_BASE), \
MT_DEVICE|MT_RW|MT_SECURE)

#define MAP_SHARED_RAM	MAP_REGION_FLAT( \
SHARED_RAM_BASE,			\
SHARED_RAM_SIZE,			\
MT_DEVICE|MT_RW|MT_SECURE)

#define MEMMAP_FIP_BASE				DDR_BASE
#define MEMMAP_FIP_SIZE				(1024*1024U)

#define BL33_IMAGE_OFFSET			(0x42000000)
#define BL33_IMAGE_MAX_SIZE			(1024*1024U)

#define PLAT_UART_BASE				UART0_BASE
#define PLAT_UART_CLK_IN_HZ			get_clk(CLK_UART)
#define PLAT_CONSOLE_BAUDRATE		U(115200)

#define PLAT_MMC_CLK_IN_HZ          get_clk(CLK_EMMC_2X)

#define ITScount 1
#define RDcount  4
#define GICD_BASE	GIC600_BASE
#define GICR_BASE	(GIC600_BASE + ((4 + (2 * ITScount)) << 16))

#if defined(IMAGE_BL1)
#	define SPI_CLK_DIV		10
#elif defined(IMAGE_BL2)
#	define SPI_CLK_DIV		10
#else
#endif

#ifdef SEEHI_SECUREBOOT
#	define BL2_SIG_ID		(MAX_IMAGE_IDS + 1)
#	define BL31_SIG_ID		(MAX_IMAGE_IDS + 2)
#	define UUID_BL2_SIG 	{{0x2A, 0x56, 0xF6, 0xC3}, {0x94, 0x7A}, {0x49, 0x08}, 0x9C, 0x9B, {0xD4, 0x9F, 0x9D, 0xAE, 0x05, 0xAB}}
#	define UUID_BL31_SIG	{{0x44, 0x83, 0x29, 0x4E}, {0xDD, 0x90}, {0x4D, 0x96}, 0x95, 0x05, {0x02, 0xAD, 0x3A, 0xA0, 0x31, 0xEB}}
#endif

#ifdef NEED_BL31_1
#	define BL31_1_IMAGE_ID		(MAX_IMAGE_IDS + 3)
#	define BL31_1_IMAGE_BASE	RESERVED_RAM_BASE
#	define BL31_1_IMAGE_LIMIT	(RESERVED_RAM_BASE + RESERVED_RAM_SIZE)
#	define UUID_BL31_1			{{0xac, 0x59, 0x4b, 0x3e}, {0xf3, 0x94}, {0x43, 0x7b}, 0xb8, 0x32, {0x3b, 0xf7, 0x80, 0xfc, 0xef, 0xdf}}
#endif

#endif /* PLATFORM_DEF_H */
