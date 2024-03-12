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
#include <plat/machine/devices_gen.h>

static inline uint64_t riscv_read_cycle(void)
{
    return riscv_read_csr_cycle();
}

static inline uint64_t riscv_read_instret(void)
{
    return riscv_read_csr_instret();
}
