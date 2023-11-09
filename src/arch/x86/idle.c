/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <util.h>

/*
 * The idle thread does not have a dedicated stack and runs in
 * the context of the idle thread TCB. Make sure that the compiler
 * always eliminates the function prologue by declaring the
 * idle_thread with the naked attribute.
 */
__attribute__((naked)) NORETURN void idle_thread(void)
{
    /* We cannot use for-loop or while-loop here because they may
     * involve stack manipulations (the compiler will not allow
     * them in a naked function anyway). */
    asm volatile(
        "1: hlt\n"
        "jmp 1b"
    );
}
