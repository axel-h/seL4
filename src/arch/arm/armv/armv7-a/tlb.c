/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <arch/machine/hardware.h>

void lockTLBEntry(vptr_t vaddr)
{

#if defined(CONFIG_ARM_CORTEX_A8)

    /* Compute two values, x and y, to write to the lockdown register. */
    static word_t tlbLockCount = 0;
    /* Before lockdown, base = victim = num_locked_tlb_entries. */
    word_t x = 1 | (tlbLockCount << 22) | (tlbLockCount << 27);
    tlbLockCount++;
    /* After lockdown, base = victim = num_locked_tlb_entries + 1. */
    word_t y = (tlbLockCount << 22) | (tlbLockCount << 27);
    lockTLBEntryCritical(vaddr, x, y);

#elif defined(CONFIG_ARM_CORTEX_A9)

    /* Compute two values, x and y, to write to the lockdown register. */
    static word_t tlbLockCount = 0;
    /* Before lockdown, victim = num_locked_tlb_entries. */
    word_t x = 1 | (tlbLockCount << 28);
    tlbLockCount++;
    /* After lockdown, victim = num_locked_tlb_entries + 1. */
    word_t y = (tlbLockCount << 28);
    lockTLBEntryCritical(vaddr, x, y);

#elif defined(CONFIG_ARM_CORTEX_A15) || defined(CONFIG_ARM_CORTEX_A7)

    /* The hardware does not support tlb locking */

#else

#error "Undefined CPU for TLB lockdown"

#endif /* A8/A9 */

}
