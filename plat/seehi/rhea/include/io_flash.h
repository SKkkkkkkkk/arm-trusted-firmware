/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IO_MYMEMMAP_H
#define IO_MYMEMMAP_H

#include <drivers/io/io_driver.h>
#include <nor_flash.h>


#define UNKNOW_FLASH_MODEL 0x0U

typedef struct {
	spi_id_t spi_id;
	bool map_mode;
	uint16_t clk_div;
	uint8_t spi_mode;
	flash_model_t flash_model;
} io_flash_dev_init_spec_t;

typedef struct {
	io_block_spec_t flash_block;
} io_flash_io_open_spec_t;

struct io_dev_connector;

int register_io_dev_flash(const struct io_dev_connector **dev_con);

static inline bool is_boot_from_flash()
{
	return (*(volatile uint32_t*)(SYSCTRL_BASE + 0xC04)) & 1;
}

#endif /* IO_MEMMAP_H */
