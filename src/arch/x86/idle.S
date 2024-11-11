/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <machine/assembler.h>

/* The idle thread does not have any stack, thus its simple loop is implemented
 * in assembly. That avoids having to find ways to convince the compiler not to
 * ever generate any code that uses the stack. There seems no clean and portable
 * way for this, and there is no guarantee any solution in C will always work.
 */
.section .text, "ax"
BEGIN_FUNC(idle_thread)
1:  hlt
    jmp 1b
END_FUNC(idle_thread)
