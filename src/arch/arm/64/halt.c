/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

void NORETURN Arch_halt(void)
{
    /* Disable interrupts and loop over WFI.
     * ToDo: If the platform runs ATF, we could issue the PSCI call CPU_OFF. The
     *       other nodes are still running, currently there is no way to halt
     *       them also. ATF defines a PSCI call SYSTEM_OFF that might be used.
     */
    MSR("daif", (DAIF_DEBUG | DAIF_SERROR | DAIF_IRQ | DAIF_FIRQ));
    for (;;) {
        wfi();
    }

    UNREACHABLE();
}
