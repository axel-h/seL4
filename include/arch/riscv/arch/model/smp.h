/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <model/smp.h>
#include <kernel/stack.h>
#include <arch/model/statedata.h>

#ifdef ENABLE_SMP_SUPPORT

extern char kernel_stack_alloc[CONFIG_MAX_NUM_NODES][BIT(CONFIG_KERNEL_STACK_BITS)];
compile_assert(kernel_stack_4k_aligned, KERNEL_STACK_ALIGNMENT == 4096)

static inline word_t hartIDToCoreID(word_t hart_id)
{
    for (word_t i = 0; i < ARRAY_SIZE(coreMap.cores); i++) {
        if (coreMap.cores[i].hart_id == hart_id) {
            return i;
        }
    }
    printf("ERROR: no entry in coreMap for hart_id %"SEL4_PRIx_word"\n", hart_id);
    halt();
}

static inline void add_hart_to_core_map(word_t hart_id, word_t core_id)
{
    if (core_id >= ARRAY_SIZE(coreMap.cores)) {
        printf("ERROR: coreMap too small to add core_id %"SEL4_PRIu_word"\n", core_id);
        halt();
    }
    coreMap.cores[core_id].hart_id = hart_id;
}

static inline bool_t try_arch_atomic_exchange_rlx(void *ptr, void *new_val, void **prev)
{
    *prev = __atomic_exchange_n((void **)ptr, new_val, __ATOMIC_RELAXED);
    return true;
}

static inline CONST cpu_id_t getCurrentCPUIndex(void)
{
    word_t sp;
    asm volatile("csrr %0, sscratch" : "=r"(sp));
    sp -= (word_t)kernel_stack_alloc;
    sp -= 8;
    return (sp >> CONFIG_KERNEL_STACK_BITS);
}

#endif /* ENABLE_SMP_SUPPORT */
