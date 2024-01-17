/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <sel4/config.h>

/* Set ENABLE_SMP_SUPPORT for kernel source files */
#ifdef CONFIG_ENABLE_SMP_SUPPORT
#define ENABLE_SMP_SUPPORT
#endif

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
#ifdef CONFIG_ARCH_AARCH64
#if (CONFIG_PHYS_ADDR_SPACE_BITS == 40)
#define AARCH64_VSPACE_S2_START_L1
#elif (CONFIG_PHYS_ADDR_SPACE_BITS == 44)
/* translation tables start at level 0 */
#else
#error "unknown CONFIG_PHYS_ADDR_SPACE_BITS"
#endif
#endif /* CONFIG_ARCH_AARCH64 */
#endif /* CONFIG_ARM_HYPERVISOR_SUPPORT */
