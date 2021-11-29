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
extern word_t ksLogIndex = 0;
extern word_t ksLogIndexFinalized = 0;
extern paddr_t ksUserLogBuffer;
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#if CONFIG_MAX_NUM_TRACE_POINTS > 0

extern timestamp_t ksEntries[CONFIG_MAX_NUM_TRACE_POINTS];
extern bool_t ksStarted[CONFIG_MAX_NUM_TRACE_POINTS];
extern timestamp_t ksExit;

#define TRACE_POINT_START(x) trace_point_start(x)
#define TRACE_POINT_STOP(x)   trace_point_stop(x)

#define MAX_LOG_SIZE (seL4_LogBufferSize / sizeof(benchmark_tracepoint_log_entry_t))

static inline void trace_point_start(word_t id)
{
    ksEntries[id] = timestamp();
    ksStarted[id] = true;
}

static inline void trace_point_stop(word_t id)
{
    benchmark_tracepoint_log_entry_t *ksLog = (benchmark_tracepoint_log_entry_t *) KS_LOG_PPTR;
    ksExit = timestamp();

    if (likely(ksUserLogBuffer != 0)) {
        if (likely(ksStarted[id])) {
            ksStarted[id] = false;
            if (likely(ksLogIndex < MAX_LOG_SIZE)) {
                ksLog[ksLogIndex] = (benchmark_tracepoint_log_entry_t) {
                    id, ksExit - ksEntries[id]
                };
            }
            /* increment the log index even if we have exceeded the log size
             * this is so we can tell if we need a bigger log */
            ksLogIndex++;
        }
        /* If this fails integer overflow has occurred. */
        assert(ksLogIndex > 0);
    }
}

#else

#define TRACE_POINT_START(x)
#define TRACE_POINT_STOP(x)

#endif /* CONFIG_MAX_NUM_TRACE_POINTS > 0 */

#endif /* CONFIG_ENABLE_BENCHMARKS */
