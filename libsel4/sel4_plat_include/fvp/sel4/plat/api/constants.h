/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>

/* The FVP can emulate different cores */
#if defined(CONFIG_ARM_CORTEX_A57)
#include <sel4/arch/constants-cortex-a57.h>
#else
#error "unsupported core"
#enidf
