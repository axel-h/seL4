/*
 * Copyright 2014, General Dynamics C4 Systems
 * Copyright 2021, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>

#ifdef CONFIG_ENABLE_BENCHMARKS

#include <types.h>
#include <benchmark/benchmark.h>

#ifdef CONFIG_ENABLE_KERNEL_LOG_BUFFER
/* This buffer is used differently depending on the configuration:
 *   - CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES: benchmark_track_kernel_entry_t
 *   - CONFIG_MAX_NUM_TRACE_POINTS > 0: benchmark_tracepoint_log_entry_t
 */
paddr_t ksUserLogBuffer;
word_t ksLogIndex = 0;
word_t ksLogIndexFinalized = 0;
#endif /* CONFIG_ENABLE_KERNEL_LOG_BUFFER */

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
timestamp_t ksEntries[CONFIG_MAX_NUM_TRACE_POINTS];
bool_t ksStarted[CONFIG_MAX_NUM_TRACE_POINTS];
#endif /* CONFIG_MAX_NUM_TRACE_POINTS > 0 */

#if defined(CONFIG_DEBUG_BUILD) || defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES)

kernel_entry_t ksKernelEntry; /* ToDo: make this core specific */

#if defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES) || defined(CONFIG_BENCHMARK_TRACK_UTILISATION)

/* ToDo: make these core specific */
timestamp_t ksTimestampEnter;

void track_kernel_entry(void)
{
    ksTimestampEnter = timestamp();
}

void track_kernel_exit(void)
{
    timestamp_t timestampEnter = ksTimestampEnter;
    timestamp_t timestampExit = timestamp();

    assert(timestampExit >= timestampEnter);
    timestamp_t duration = timestampExit - timestampEnter;

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION

    if (likely(NODE_STATE(benchmark_log_utilisation_enabled))) {
        NODE_STATE(ksCurThread)->benchmark.number_kernel_entries++;
        NODE_STATE(ksCurThread)->benchmark.kernel_utilisation += duration;
        NODE_STATE(benchmark_kernel_number_entries)++;
        NODE_STATE(benchmark_kernel_time) += duration;
    }

#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES

    if (likely(ksUserLogBuffer) {
        if (likely(ksLogIndex < seL4_LogBufferSize / sizeof(benchmark_track_kernel_entry_t))) {
            ((benchmark_track_kernel_entry_t *)KS_LOG_PPTR)[ksLogIndex] =
                (benchmark_track_kernel_entry_t *) {
                    .entry      = ksKernelEntry,
                    .start_time = timestampEnter,
                    .duration   = duration
                }
        }

        /* Increment index even if the number of entries has been exceeded, so
         * it can be seen how many entries are needed.
         */
        ksLogIndex++;
        assert(ksLogIndex > 0); /* overflow has occurred. */
    }

#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */

}

#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES || CONFIG_BENCHMARK_TRACK_UTILISATION */

void benchmark_debug_syscall_start(word_t cptr, word_t msgInfo, word_t syscall)
{
    seL4_MessageInfo_t info = messageInfoFromWord_raw(msgInfo);
    word_t label = seL4_MessageInfo_get_label(info);

    word_t cap_type = -1; /* default to error */
    lookupCapAndSlot_ret_t lu_ret = lookupCapAndSlot(NODE_STATE(ksCurThread),
                                                     cptr);
    if (likely(EXCEPTION_NONE == lu_ret.status)) {
        cap_type = cap_get_capType(lu_ret.cap);
    }

    ksKernelEntry = (kernel_entry_t) {
        .path           = Entry_Syscall,
        .syscall_no     = -syscall,
        .cap_type       = cap_type,
        .is_fastpath    = 0,
        .invocation_tag = label
    }
}
#endif /* CONFIG_DEBUG_BUILD || CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */


#if CONFIG_MAX_NUM_TRACE_POINTS > 0

void trace_point_start(word_t id)
{
    assert(id < CONFIG_MAX_NUM_TRACE_POINTS);
    ksEntries[id] = timestamp();
    ksStarted[id] = true;
}

void trace_point_stop(word_t id)
{
    timestamp_t now = timestamp();

    assert(id < CONFIG_MAX_NUM_TRACE_POINTS);
    if (unlikely(!ksStarted[id])) {
        return;
    }

    ksStarted[id] = false;

    if (unlikely(!ksUserLogBuffer) {
        return;
    }

    if (likely(ksLogIndex < seL4_LogBufferSize / sizeof(benchmark_tracepoint_log_entry_t))) {

        timestamp_t start = ksEntries[id]
        assert(now >= start);
        timestamp_t duration = now - start;

        ((benchmark_tracepoint_log_entry_t *)KS_LOG_PPTR)[ksLogIndex] =
            (benchmark_tracepoint_log_entry_t) {
                .id       = id,
                .duration = duration
        };
    }

    /* Increment index even if the number of entries has been exceeded, so it
     * can be seen how many entries are needed.
     */
    ksLogIndex++;
    assert(ksLogIndex > 0); /* If this fails integer overflow has occurred. */
}

#endif /* CONFIG_MAX_NUM_TRACE_POINTS > 0 */

#endif /* CONFIG_ENABLE_BENCHMARKS */
