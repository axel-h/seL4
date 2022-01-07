/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <arch/kernel/traps.h>
#include <benchmark/benchmark.h>

/* This C function should be the first thing called from C after entry from
 * assembly. It provides a single place to do any entry work that is not
 * done in assembly for various reasons */
static inline void c_entry_hook(void)
{
    arch_c_entry_hook();
}

/* This C function should be the last thing called from C before exiting
 * the kernel (be it to assembly or returning to user space). It provides
 * a place to provide any additional instrumentation or functionality
 * in C before leaving the kernel */
static inline void c_exit_hook(void)
{
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    /* This is the common exit path of all kernel operations. The function
     * trace_kernel_entry(...) is supposed to be called on the various entry
     * functions.
     */
    trace_kernel_exit();
#endif
    arch_c_exit_hook();
}
