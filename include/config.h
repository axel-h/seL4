/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <sel4/config.h>

#ifdef CONFIG_ENABLE_SMP_SUPPORT
#define ENABLE_SMP_SUPPORT
#define SMP_TERNARY(_smp, _up)      _smp
#if defined(CONFIG_DEBUG_BUILD) && defined(CONFIG_KERNEL_MCS) && !defined(CONFIG_PLAT_QEMU_ARM_VIRT)
#define ENABLE_SMP_CLOCK_SYNC_TEST_ON_BOOT
#endif
#else
#define SMP_TERNARY(_smp, _up)      _up
#endif /* CONFIG_ENABLE_SMP_SUPPORT */

#define SMP_COND_STATEMENT(_st)     SMP_TERNARY(_st,)
#define UP_COND_STATEMENT(_st)      SMP_TERNARY(,_st)

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
#ifdef CONFIG_ARM_PA_SIZE_BITS_40
#define AARCH64_VSPACE_S2_START_L1
#endif
#endif
