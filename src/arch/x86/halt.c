/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <util.h>

void NORETURN Arch_halt(void)
{
    /* Disable interrupts and loop over HLT on this core.
     * ToDo: The other nodes are still running, currently there is no way to
     *       halt them also.
     */
    asm volatile("cli");
    for (;;) {
        asm volatile("hlt" ::: "memory");
    }

    UNREACHABLE();
}
