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

/* Read a consistent 64-bit counter value from two 32-bit registers. The value
 * from the low register is used only if there was no roll over, otherwise it is
 * simply taken as 0. This is an acceptable optimization if the value must have
 * been 0 at some point anyway and certain jitter is acceptable. For high
 * frequency counters the preference usually not on getting an exact value, but
 * a value close to the point in time where the read function was called. For a
 * low frequency counter, the low value is likely 0 anyway when a roll over
 * happens.
 */
#define declare_helper_get_riscv_counter_csr64(_name_, _id_hi_, _id_lo_) \
    static inline uint64_t get_riscv_counter_csr64_##_name_(void) \
    { \
        register word_t nH_prev, nH, nL; \
        RISCV_CSR_READ(_id_hi_, nH_prev); \
        RISCV_CSR_READ(_id_lo_, nL); \
        RISCV_CSR_READ(_id_hi_, nH); \
        if (nH_prev != nH) { RISCV_CSR_READ(_id_lo_, nL); } \
        return (((uint64_t)nH) << 32) | nL; \
    }

/* create get_riscv_counter_csr64_time() */
declare_helper_get_riscv_counter_csr64(time, RISCV_CSR_TIMEH,
                                       RISCV_CSR_TIME)

/* create get_riscv_counter_csr64_cycle() */
declare_helper_get_riscv_counter_csr64(cycle, RISCV_CSR_CYCLEH,
                                       RISCV_CSR_CYCLE)

/* create get_riscv_counter_csr64_instret() */
declare_helper_get_riscv_counter_csr64(instret, RISCV_CSR_INSTRETH,
                                       RISCV_CSR_INSTRET)


#ifdef CONFIG_RISCV_USE_CLINT_MTIME
/*
 * Currently all RISC-V 32-bit platforms supported have the mtime register
 * mapped at the same offset of the base address of the CLINT.
 */
#define CLINT_MTIME_OFFSET_LO 0xbff8
#define CLINT_MTIME_OFFSET_HI 0xbffc

static inline uint32_t riscv_read_clint_u32(word_t offset)
{
    return *(volatile uint32_t *)(CLINT_PPTR + offset);
}

static inline uint64_t riscv_read_clint_mtime(void)
{
    /*
     * Ensure that the time is correct if there is a rollover in the
     * high bits between reading the low and high bits.
     */
    uint32_t nH_prev = riscv_read_clint_u32(CLINT_MTIME_OFFSET_HI);
    uint32_t nL = riscv_read_clint_u32(CLINT_MTIME_OFFSET_LO);
    uint32_t nH = riscv_read_clint_u32(CLINT_MTIME_OFFSET_HI);
    if (nH_prev != nH) {
        nL = riscv_read_clint_u32(CLINT_MTIME_OFFSET_LO);
    }
    return (((uint64_t)nH) << 32) | nL;
}

#endif /* CONFIG_RISCV_USE_CLINT_MTIME */

static inline uint64_t riscv_read_time(void)
{
#ifdef CONFIG_RISCV_USE_CLINT_MTIME
    return riscv_read_clint_mtime();
#else
    return get_riscv_counter_csr64_time();
#endif
}

static inline uint64_t riscv_read_cycle(void)
{
    return get_riscv_counter_csr64_cycle();
}

static inline uint64_t riscv_read_instret(void)
{
    return get_riscv_counter_csr64_instret();
}
