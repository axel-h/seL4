/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <mode/smp/smp.h>
#include <model/smp.h>

#ifdef ENABLE_SMP_SUPPORT
static inline cpu_id_t cpuIndexToID(word_t index)
{
    return BIT(index);
}

static inline bool_t try_arch_atomic_exchange_rlx(void *ptr, void *new_val, void **prev)
{
    /* The ARM architecture had an atomic swap instruction in ARMv6, which got
     * deprecated and war replaced by exclusive load and store instructions in
     * ARMv7. The compiler builtin __atomic_exchange_n() executed them in a loop
     * as long as the exclusivity fail fails. Unfortunately, this approach can
     * results in an undefined delay up to seconds in the worst case if other
     * cores touch memory in the same exclusivity reservation granule. The
     * actual granule size is implementation specific. The explicit load and
     * store instruction below and reporting back success or failure is the only
     * way to work around this, the caller can loop over calling this functions
     * this and perform other maintenance operation or wait on failure.
     * ARM v8.1 eventually brought the atomic swap instruction back as part of
     * LSE (Large System Extensions) and GCC support was added in v9.4. Thus,
     * using __atomic_exchange_n() becomes an option when "-march=armv8.1-a" is
     * passed on platform that use core implementing ARMv8.1 or higher. Details
     * remain to be analyzed.
     */
    uint32_t atomic_status;
    void *temp;

    asm volatile(
        LD_EX "%[prev_output], [%[ptr_val]]             \n\t" /* ret = *ptr */
        ST_EX "%" OP_WIDTH "[atomic_var], %[new_val] , [%[ptr_val]] \n\t"  /* *ptr = new */
        : [atomic_var] "=&r"(atomic_status), [prev_output]"=&r"(temp)     /* output */
        : [ptr_val] "r"(ptr), [new_val] "r"(new_val)   /* input */
        :
    );

    if (0 != atomic_status) {
        /* Specs say 0 indicates success and 1 a exclusivity failure, any other
         * value is not defined. If the atomic operation has failed, prev is
         * left untouched. There seems no gain updating it with the value
         * obtained, as this might no longer be valid anyway.
         */
        return false;
    }

    *prev = temp;
    return true;
}

#endif /* ENABLE_SMP_SUPPORT */

