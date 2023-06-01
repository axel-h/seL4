/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <arch/machine/hardware.h>

void lockTLBEntry(vptr_t vaddr)
{

#if defined(CONFIG_ARM_CORTEX_A53) || defined(CONFIG_ARM_CORTEX_A55) || \
    defined(CONFIG_ARM_CORTEX_A57) || defined(CONFIG_ARM_CORTEX_A72)

    /* On VMSAv8-32, the CP15 c10 register has reserved encodings for
     * IMPLEMENTATION DEFINED TLB control functions, including lockdown.
     *
     * The known hardware does not support tlb locking
     */

#else

#error "Undefined CPU for TLB lockdown"

#endif /* A8/A9 */

}
