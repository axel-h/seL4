/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <model/smp.h>
#include <kernel/stack.h>

#ifdef ENABLE_SMP_SUPPORT

typedef struct {
    word_t *user_thread_regs;
    word_t reserved;
    word_t core_id;
    word_t hart_id;
} core_info_t;


typedef struct core_map {
    word_t map[CONFIG_MAX_NUM_NODES];
} core_map_t;

extern char kernel_stack_alloc[CONFIG_MAX_NUM_NODES][BIT(CONFIG_KERNEL_STACK_BITS)];
compile_assert(kernel_stack_4k_aligned, KERNEL_STACK_ALIGNMENT == 4096)
extern core_map_t coreMap;

static inline cpu_id_t cpuIndexToID(word_t index)
{
    assert(index < CONFIG_MAX_NUM_NODES);
    return coreMap.map[index];
}

static inline word_t hartIDToCoreID(word_t hart_id)
{
    word_t i = 0;
    for (i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
        if (coreMap.map[i] == hart_id) {
            break;
        }
    }
    return i;
}

static inline void add_hart_to_core_map(word_t hart_id, word_t core_id)
{
    assert(core_id < CONFIG_MAX_NUM_NODES);
    coreMap.map[core_id] = hart_id;
}

static inline bool_t try_arch_atomic_exchange_rlx(void *ptr, void *new_val, void **prev)
{
    *prev = __atomic_exchange_n((void **)ptr, new_val, __ATOMIC_RELAXED);
    return true;
}


/* We can't include arch/machine.h because there is a circular include
 * dependency. Until that can be resolved, we need to define the function
 * prototype here.
 */
static inline word_t read_sscratch(void);

static inline CONST cpu_id_t getCurrentCPUIndex(void)
{
    /* RISC-V has no dedicated S-Mode register for the current hart ID, this
     * information is passed from the bootloader to the kernel, which is
     * supposed to store it somewhere. We explicitly store it, but keep it
     * implicitly in the core specific memory page, which is kept in SSCRATCH.
     *
     *
     *     0x1000   +---------------------------------+
     *              |                                 |
     *              | Kernel Stack                    |
     *              |                                 |
     *              +---------------------------------+  <------ SSCRATCH
     *              | ptr to user thread regs         | word_t
     *              +---------------------------------+
     *              | (reserved for 64-bit alignment) | word_t
     *              +---------------------------------+
     *              | Logical Core Number             | word_t
     *              +---------------------------------+
     *              | Hard ID                         | word_t
     *     0x0000   +---------------------------------+
     */
    core_info_t *info = (core_info_t)read_sscratch();
    word_t idx = info->core_id;
    assert(idx < CONFIG_MAX_NUM_NODES);
    return idx;
}

#endif /* ENABLE_SMP_SUPPORT */


