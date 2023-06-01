/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <arch/machine/hardware.h>

void lockTLBEntry(vptr_t vaddr)
{
    /* The hardware does not support tlb locking */
}
