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
#ifdef CONFIG_ARM_PA_SIZE_BITS_40
#define AARCH64_VSPACE_S2_START_L1
#endif
#endif

#if defined(CONFIG_DEBUG_BUILD) || \
    defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES)
#define TRACK_KERNEL_ENTRY_DETAILS
//#define ENABLE_TRACE_KERNEL_ENTRY
#define TRACK_KERNEL_ENTRIES /* ToDo: remove this define */
#endif

#if defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES) || \
    defined(CONFIG_BENCHMARK_TRACK_UTILISATION) || \
    defined(TRACK_KERNEL_ENTRY_DETAILS)
#define ENABLE_TRACE_KERNEL_ENTRY_EXIT
#endif

#if defined(CONFIG_BENCHMARK_TRACEPOINTS) && (CONFIG_MAX_NUM_TRACE_POINTS > 0)
#define ENABLE_KERNEL_TRACEPOINTS
#endif
