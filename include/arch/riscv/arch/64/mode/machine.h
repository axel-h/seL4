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

#ifdef CONFIG_RISCV_USE_CLINT_MTIME
/*
 * Currently all RISC-V 64-bit platforms supported have the mtime register
 * mapped at the same offset of the base address of the CLINT.
 */
#define CLINT_MTIME_OFFSET 0xbff8

static inline uint64_t riscv_read_clint_u64(word_t offset)
{
    return *(volatile uint64_t *)(CLINT_PPTR + offset);
}

static inline uint64_t riscv_read_clint_mtime(void)
{
    return riscv_read_clint_u64(CLINT_MTIME_OFFSET);
}

#endif /* CONFIG_RISCV_USE_CLINT_MTIME */

static inline uint64_t riscv_read_time(void)
{
#ifdef CONFIG_RISCV_USE_CLINT_MTIME
    return riscv_read_clint_mtime();
#else
    return get_riscv_csr_time();
#endif
}

static inline uint64_t riscv_read_cycle(void)
{
    return get_riscv_csr_cycle();
}

static inline uint64_t riscv_read_instret(void)
{
    return get_riscv_csr_instret();
}
