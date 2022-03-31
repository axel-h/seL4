/*
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <util.h>
#include <arch/sbi.h>

void NORETURN Arch_halt(void)
{
    /* Use SBI to halt the system.
     * ToDo: Systems may not have an SBI, in this case disabling all interrupts
     *       an looping over WFI is the best we can do.
     */
    sbi_shutdown();

    UNREACHABLE();
}
