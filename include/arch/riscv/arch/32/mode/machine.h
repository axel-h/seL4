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
#define declare_helper_riscv_read_csr64cntr(_name_, _id_hi_, _id_lo_) \
    static inline uint64_t riscv_read_csr64cntr_##_name_(void) \
    { \
        register word_t nH_prev, nH, nL; \
        RISCV_CSR_READ(_id_hi_, nH_prev); \
        RISCV_CSR_READ(_id_lo_, nL); \
        RISCV_CSR_READ(_id_hi_, nH); \
        if (nH_prev != nH) { RISCV_CSR_READ(_id_lo_, nL); } \
        return (((uint64_t)nH) << 32) | nL; \
    }

/* create riscv_read_csr64cntr_time() */
declare_helper_riscv_read_csr64cntr(time, RISCV_CSR_TIMEH, RISCV_CSR_TIME)

/* create riscv_read_csr64cntr_cycle() */
declare_helper_riscv_read_csr64cntr(cycle, RISCV_CSR_CYCLEH, RISCV_CSR_CYCLE)

/* create get_riscv_csr64cntr_instret() */
declare_helper_riscv_read_csr64cntr(instret, RISCV_CSR_INSTRETH, RISCV_CSR_INSTRET)

static inline uint64_t riscv_read_cycle(void)
{
    return riscv_read_csr64cntr_cycle();
}

static inline uint64_t riscv_read_instret(void)
{
    return riscv_read_csr64cntr_instret();
}
