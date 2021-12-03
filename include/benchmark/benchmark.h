/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>

#ifdef CONFIG_ENABLE_BENCHMARKS

#include <types.h>
#include <arch/benchmark.h>
#include <machine/io.h>
#include <sel4/arch/constants.h>
#include <arch/machine/hardware.h>
#include <sel4/benchmark_tracepoints_types.h>
#include <mode/hardware.h>

#ifdef CONFIG_KERNEL_LOG_BUFFER
extern seL4_Word ksLogIndex = 0;
extern paddr_t ksUserLogBuffer;
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#if defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES) || defined(CONFIG_BENCHMARK_TRACK_UTILISATION)
/* Having one global kernel entry timestamp does not work in SMP configurations,
 * as the kernel could be entered in parallel on different cores. For now, it is
 * assumed that benchmarking is used on single core configurations only.
 */
extern timestamp_t ksEnter;
#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES CONFIG_BENCHMARK_TRACK_UTILISATION */

#if CONFIG_MAX_NUM_TRACE_POINTS > 0

extern timestamp_t ksEntries[CONFIG_MAX_NUM_TRACE_POINTS];
extern bool_t ksStarted[CONFIG_MAX_NUM_TRACE_POINTS];

#define TRACE_POINT_START(x)    trace_point_start(x)
#define TRACE_POINT_STOP(x)     trace_point_stop(x)

static inline void trace_point_start(word_t id)
{
    ksEntries[id] = timestamp();
    ksStarted[id] = true;
}

static inline void trace_point_stop(word_t id)
{
    benchmark_tracepoint_log_entry_t *ksLog = (benchmark_tracepoint_log_entry_t *) KS_LOG_PPTR;
    timestamp_t now = timestamp();

    if (likely(ksUserLogBuffer != 0)) {
        if (likely(ksStarted[id])) {
            ksStarted[id] = false;
            if (likely(ksLogIndex < (seL4_LogBufferSize / sizeof(*ksLog)))) {
                ksLog[ksLogIndex] = (benchmark_tracepoint_log_entry_t) {
                    .id = id,
                    .duration = now - ksEntries[id]
                };
            }
            /* Increment the log index even if we have exceeded the actual log
             * size, so we can tell if we need a bigger log.
             */
            ksLogIndex++;
        }
        /* If this fails an integer overflow has occurred. */
        assert(ksLogIndex > 0);
    }
}

#else

#define TRACE_POINT_START(x)
#define TRACE_POINT_STOP(x)

#endif /* CONFIG_MAX_NUM_TRACE_POINTS > 0 */

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
