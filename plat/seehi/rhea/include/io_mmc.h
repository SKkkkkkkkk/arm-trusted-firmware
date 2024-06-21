/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IO_mmc_H
#define IO_mmc_H

#include <drivers/io/io_driver.h>

typedef struct {
	int idx;
} io_mmc_dev_spec_t;

typedef struct {
	io_block_spec_t mmc_block;
	size_t block_size;
} io_mmc_io_open_spec_t;

int register_io_dev_mmc(const struct io_dev_connector **dev_con);

#endif /* IO_mmc_H */
