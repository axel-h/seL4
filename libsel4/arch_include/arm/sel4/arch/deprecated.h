/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/config.h>

#if (CONFIG_PHYS_ADDR_SPACE_BITS == 40)
#define ARM_PA_SIZE_BITS_40
#elif (CONFIG_PHYS_ADDR_SPACE_BITS == 44)
#define ARM_PA_SIZE_BITS_44
#else
#error "Unknown physical address width"
#endif
