/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <machine/assembler.h>

/* The idle thread does not have any stack, thus this loop is implemented as
 * assembler function. It's easier than convincing the compiler not to use any
 * stack for a C function.
 */
.section ._idle_thread
BEGIN_FUNC(idle_thread)
1:  hlt
    jmp 1b
END_FUNC(idle_thread)
