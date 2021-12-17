/*
 * Copyright 2014, General Dynamics C4 Systems
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <util.h>

void NORETURN Arch_halt(void)
{
    /* Disable interrupts and loop over WFI.
     * ToDo: If the platform runs ATF, we could issue the PSCI call CPU_OFF. The
     *       other nodes are still running, currently there is no way to halt
     *       them also. ATF defines a PSCI call SYSTEM_OFF.
     */
    asm volatile("cpsid iaf");
    for (;;) {
        wfi();
    }

    UNREACHABLE();
}
