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

#ifdef CONFIG_BKL_SWAP_MANUAL
static inline void *arch_atomic_exchange(void *head, void *node)
{
    /* ARMv6 had an atomic swap instruction. ARMv7 deprecated is and recommended
     * using exclusive load and store instructions, which the compiler builtin
     * __atomic_exchange_n() executes in a loop as long as the exclusivity fail
     * fails. Unfortunately, this approach can results in an undefined delay up
     * to seconds in the worst case if other cores touch memory in the same
     * exclusivity reservation granule. The actual granule size is
     * implementation specific, for modern SMP optimized cores the size is
     * expected to be small.
     * ARM v8.1 eventually brought the atomic swap instruction back as part of
     * LSE (Large System Extensions) and GCC support was added in v9.4. Thus,
     * using __atomic_exchange_n() becomes an option when "-march=armv8.1-a" is
     * passed on platform that use core implementing ARMv8.1 or higher. Details
     * remain to be analyzed.
     */
    __atomic_thread_fence(__ATOMIC_RELEASE); /* all writes must finish */

    for (;;)  {
        void *prev;
        uint32_t atomic_status;
        asm volatile(
            LD_EX "%[prev_output], [%[ptr_val]]                         \n\t" /* prev = *head */
            ST_EX "%" OP_WIDTH "[atomic_var], %[new_val] , [%[ptr_val]] \n\t" /* *head = node */
            : /* output */ [atomic_var] "=&r"(atomic_status), [prev_output]"=&r"(prev)
            : /* input */ [ptr_val] "r"(head), [new_val] "r"(node)
            : /* no clobber */
        );

        /* Specs say 0 indicates success and 1 an exclusivity failure, any other
         * value is not defined.
         */
        if (0 == atomic_status) {
            /* The write for the update of the queue write has finished anyway,
             * prevent pipeline from starting any reads before passing here
             */
            __atomic_thread_fence(__ATOMIC_ACQUIRE);
            return prev;
        }

        /* keep spinning */
    }

}
#endif /* CONFIG_BKL_SWAP_MANUAL */

#endif /* ENABLE_SMP_SUPPORT */

