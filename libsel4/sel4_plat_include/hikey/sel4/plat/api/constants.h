/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/config.h>
/* HiKey uses a HiSilicon Kirin620 SoC with 2 clusters of 4 Cortex-A53 each. */
#include <sel4/arch/constants_cortex_a53.h>

#if CONFIG_WORD_SIZE == 32
/* First address in the virtual address space that is not accessible to user level */
#define seL4_UserTop 0xe0000000
#elif CONFIG_WORD_SIZE == 64
/*  defined at the arch level */
#else
#error "unsupported CONFIG_WORD_SIZE"
#endif
