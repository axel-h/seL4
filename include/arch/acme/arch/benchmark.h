/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <types.h>
#include <hardware.h>

#ifdef CONFIG_ENABLE_BENCHMARKS
static inline timestamp_t timestamp(void)
{
    return acme_read_cycle();
}

static inline void benchmark_arch_utilisation_reset(void)
{
    /* nothing here */
}

#endif /* CONFIG_ENABLE_BENCHMARK */

