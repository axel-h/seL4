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

/* create get_riscv_csr_timeh() */
declare_helper_get_riscv_csr(timeh, RISCV_CSR_TIMEH)
/* create get_riscv_csr64_timeh_time() */
declare_helper_get_riscv_counter_csr64(timeh_time, RISCV_CSR_TIMEH,
                                       RISCV_CSR_TIME)

/* create get_riscv_csr_cycleh() */
declare_helper_get_riscv_csr(cycleh, RISCV_CSR_CYCLEH)
/* create get_riscv_csr64_cycleh_cycle() */
declare_helper_get_riscv_counter_csr64(cycleh_cycle, RISCV_CSR_CYCLEH,
                                       RISCV_CSR_CYCLE)


static inline uint64_t riscv_read_time(void)
{
    return get_riscv_csr64_counter_timeh_time();
}


static inline uint64_t riscv_read_cycle(void)
{
    return get_riscv_csr64_counter_cycleh_cycle();
}
