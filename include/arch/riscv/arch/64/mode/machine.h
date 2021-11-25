/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <util.h>
#include <arch/model/smp.h>
#include <arch/machine/registerset.h>
#include <stdint.h>

static inline uint64_t riscv_read_time(void)
{
    return get_riscv_csr_time();
}

static inline uint64_t riscv_read_cycle(void)
{
    return get_riscv_csr_cycle();
}
