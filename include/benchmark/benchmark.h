/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <util.h>
#include <types.h>
#include <machine/io.h>
#include <arch/machine/hardware.h>
#include <mode/hardware.h>
#include <model/statedata.h>
#include <kernel/cspace.h>
#include <sel4/arch/constants.h>
#include <arch/benchmark.h>

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
#include <sel4/benchmark_utilisation_types.h>
#include <benchmark/benchmark_utilisation.h>
#endif

#ifdef CONFIG_TRACE_KERNEL_ENTRIES
#include <sel4/benchmark_track_types.h>
#endif

#ifdef CONFIG_KERNEL_LOG_BUFFER
extern seL4_Word ksLogIndex = 0;
extern paddr_t ksUserLogBuffer;
#endif /* CONFIG_ENABLE_KERNEL_LOG_BUFFER */

#define MAX_LOG_SIZE (CONFIG_KERNEL_LOG_BUFFER_SIZE / \
                        sizeof(benchmark_tracepoint_log_entry_t))

#ifdef TRACK_KERNEL_ENTRY_DETAILS
#include <sel4/benchmark_track_types.h>
extern kernel_entry_t ksKernelEntry;
#endif /* TRACK_KERNEL_ENTRY_DETAILS */

#ifdef ENABLE_KERNEL_TRACEPOINTS
#include <sel4/benchmark_tracepoints_types.h>
void trace_point_start(word_t id);
#define TRACE_POINT_START(id)  trace_point_start(id)
void trace_point_stop(word_t id);
#define TRACE_POINT_STOP(id)   trace_point_stop(id)
#else /* not ENABLE_KERNEL_TRACEPOINTS */
#define TRACE_POINT_START(...)  do {} while(0)
#define TRACE_POINT_STOP(...)   do {} while(0)
#endif /* [not] ENABLE_KERNEL_TRACEPOINTS */


#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
#include <sel4/benchmark_track_types.h>
#include <kernel/cspace.h>
#include <model/statedata.h>
#include <mode/machine.h>

extern kernel_entry_t ksKernelEntry;
#ifdef TRACK_KERNEL_ENTRY_DETAILS
static void trace_kernel_set_entry_reason(word_t path, word_t word)
{
    ksKernelEntry = (kernel_entry_t) {
        .path = path,
        /* 29 bits are remaining, usage specific to path */
        .core = CURRENT_CPU_INDEX(), /* 3 bits */
        .word = word, /* 26 bits */
    };
}
#endif /* TRACK_KERNEL_ENTRY_DETAILS */
void trace_kernel_entry(word_t path, word_t word);
void trace_kernel_entry_syscall(word_t id, word_t cptr, word_t msgInfo,
                                word_t isFastpath);
void trace_kernel_exit(void);
#endif /* ENABLE_TRACE_KERNEL_ENTRY_EXIT */

#ifdef CONFIG_ENABLE_BENCHMARKS
exception_t handle_SysBenchmarkFlushCaches(void);
exception_t handle_SysBenchmarkResetLog(void);
exception_t handle_SysBenchmarkFinalizeLog(void);
#ifdef CONFIG_ENABLE_KERNEL_LOG_BUFFER
exception_t handle_SysBenchmarkSetLogBuffer(void);
#endif /* CONFIG_ENABLE_KERNEL_LOG_BUFFER */
#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
exception_t handle_SysBenchmarkGetThreadUtilisation(void);
exception_t handle_SysBenchmarkResetThreadUtilisation(void);
#ifdef CONFIG_DEBUG_BUILD
exception_t handle_SysBenchmarkDumpAllThreadsUtilisation(void);
exception_t handle_SysBenchmarkResetAllThreadsUtilisation(void);
#endif /* CONFIG_DEBUG_BUILD */
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */
#endif /* CONFIG_ENABLE_BENCHMARKS */
