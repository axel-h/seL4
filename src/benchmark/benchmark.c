/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <types.h>
#include <mode/machine.h>
#include <benchmark/benchmark.h>
#include <benchmark/benchmark_utilisation.h>

#ifdef CONFIG_KERNEL_LOG_BUFFER
/* The buffer is used differently depending on the configuration:
 *   - CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES: benchmark_track_kernel_entry_t
 *   - ENABLE_KERNEL_TRACEPOINTS: benchmark_tracepoint_log_entry_t
 */
seL4_Word ksLogIndex = 0;
paddr_t ksUserLogBuffer;
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#if defined(CONFIG_DEBUG_BUILD) || defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES)
#include <sel4/benchmark_track_types.h>
kernel_entry_t ksKernelEntry;
#endif /* CONFIG_DEBUG_BUILD || CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */

#if defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES) || defined(CONFIG_BENCHMARK_TRACK_UTILISATION)
timestamp_t ksEnter;
#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES CONFIG_BENCHMARK_TRACK_UTILISATION */


#ifdef ENABLE_KERNEL_TRACEPOINTS

static timestamp_t ksEntries[CONFIG_MAX_NUM_TRACE_POINTS];
static bool_t ksStarted[CONFIG_MAX_NUM_TRACE_POINTS];

void trace_point_start(word_t id)
{
    if (unlikely((id >= ARRAY_SIZE(ksEntries)) || (id >= ARRAY_SIZE(ksStarted)))) {
        /* The assert exists in debug builds only, in release no trace point
         * start will be recorded if an invalid ID is provided. This seems a
         * better approach than corrupting memory. Make this a fatal error that
         * halts the system even for release builds seem too radical, as this is
         * just a trace problem and not necessarily a system problem.
         */
        assert(0);
        return;
    }

    ksEntries[id] = timestamp();
    ksStarted[id] = true;
}

void trace_point_stop(word_t id)
{

    if (unlikely(id >= ARRAY_SIZE(ksStarted))) {
        /* The assert exists in debug builds only, in release we will simply
         * assume the trace point has not been started.
         */
        assert(0);
        return
    }

    if (!ksStarted[id]) {
        return
    }

    ksStarted[id] = false;
    if (unlikely(id >= ARRAY_SIZE(ksEntries))) {
        /* The assert exists in debug builds only, in release we will do
         * nothing if the ID is out of range.
         */
        assert(0);
        return;
    }

    if (likely(ksLogIndex < (seL4_LogBufferSize / sizeof(benchmark_tracepoint_log_entry_t)))) {
        timestamp_t start = ksEntries[id];
        timestamp_t now = timestamp();
        assert(now >= start);
        timestamp_t duration = now - start;

        ((benchmark_tracepoint_log_entry_t *)KS_LOG_PPTR)[ksLogIndex] =
        (benchmark_tracepoint_log_entry_t) {
            .id       = id,
            .duration = duration
        };
    }

    /* increment the log index even if we have exceeded the log size, so we can
     * tell if we need a bigger log
     */
    ksLogIndex++;
    /* If this fails, an integer overflow has occurred. */
    assert(ksLogIndex > 0);
}

#endif /* ENABLE_KERNEL_TRACEPOINTS */

#ifdef CONFIG_ENABLE_BENCHMARKS

exception_t handle_SysBenchmarkFlushCaches(void)
{
#ifdef CONFIG_ARCH_ARM
    tcb_t *thread = NODE_STATE(ksCurThread);
    if (getRegister(thread, capRegister)) {
        arch_clean_invalidate_L1_caches(getRegister(thread, msgInfoRegister));
    } else {
        arch_clean_invalidate_caches();
    }
#else
    arch_clean_invalidate_caches();
#endif
    return EXCEPTION_NONE;
}

exception_t handle_SysBenchmarkResetLog(void)
{
#ifdef CONFIG_KERNEL_LOG_BUFFER
    if (ksUserLogBuffer == 0) {
        userError("A user-level buffer has to be set before resetting benchmark.\
                Use seL4_BenchmarkSetLogBuffer\n");
        setRegister(NODE_STATE(ksCurThread), capRegister, seL4_IllegalOperation);
        return EXCEPTION_SYSCALL_ERROR;
    }

    ksLogIndex = 0;
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
    NODE_STATE(benchmark_log_utilisation_enabled) = true;
    benchmark_track_reset_utilisation(NODE_STATE(ksIdleThread));
    NODE_STATE(ksCurThread)->benchmark.schedule_start_time = ksEnter;
    NODE_STATE(ksCurThread)->benchmark.number_schedules++;
    NODE_STATE(benchmark_start_time) = ksEnter;
    NODE_STATE(benchmark_kernel_time) = 0;
    NODE_STATE(benchmark_kernel_number_entries) = 0;
    NODE_STATE(benchmark_kernel_number_schedules) = 1;
    benchmark_arch_utilisation_reset();
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */

    setRegister(NODE_STATE(ksCurThread), capRegister, seL4_NoError);
    return EXCEPTION_NONE;
}

exception_t handle_SysBenchmarkFinalizeLog(void)
{
#ifdef CONFIG_KERNEL_LOG_BUFFER
    setRegister(NODE_STATE(ksCurThread), capRegister, ksLogIndex);
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
    benchmark_utilisation_finalise();
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */

    return EXCEPTION_NONE;
}

#ifdef CONFIG_KERNEL_LOG_BUFFER
exception_t handle_SysBenchmarkSetLogBuffer(void)
{
    word_t cptr_userFrame = getRegister(NODE_STATE(ksCurThread), capRegister);
    if (benchmark_arch_map_logBuffer(cptr_userFrame) != EXCEPTION_NONE) {
        setRegister(NODE_STATE(ksCurThread), capRegister, seL4_IllegalOperation);
        return EXCEPTION_SYSCALL_ERROR;
    }

    setRegister(NODE_STATE(ksCurThread), capRegister, seL4_NoError);
    return EXCEPTION_NONE;
}
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION

exception_t handle_SysBenchmarkGetThreadUtilisation(void)
{
    benchmark_track_utilisation_dump();
    return EXCEPTION_NONE;
}

exception_t handle_SysBenchmarkResetThreadUtilisation(void)
{
    word_t tcb_cptr = getRegister(NODE_STATE(ksCurThread), capRegister);
    lookupCap_ret_t lu_ret;
    word_t cap_type;

    lu_ret = lookupCap(NODE_STATE(ksCurThread), tcb_cptr);
    /* ensure we got a TCB cap */
    cap_type = cap_get_capType(lu_ret.cap);
    if (cap_type != cap_thread_cap) {
        userError("SysBenchmarkResetThreadUtilisation: cap is not a TCB, halting");
        return EXCEPTION_NONE;
    }

    tcb_t *tcb = TCB_PTR(cap_thread_cap_get_capTCBPtr(lu_ret.cap));

    benchmark_track_reset_utilisation(tcb);
    return EXCEPTION_NONE;
}

#ifdef CONFIG_DEBUG_BUILD

exception_t handle_SysBenchmarkDumpAllThreadsUtilisation(void)
{
    printf("{\n");
    printf("  \"BENCHMARK_TOTAL_UTILISATION\":%lu,\n",
           (word_t)(NODE_STATE(benchmark_end_time) - NODE_STATE(benchmark_start_time)));
    printf("  \"BENCHMARK_TOTAL_KERNEL_UTILISATION\":%lu,\n", (word_t) NODE_STATE(benchmark_kernel_time));
    printf("  \"BENCHMARK_TOTAL_NUMBER_KERNEL_ENTRIES\":%lu,\n", (word_t) NODE_STATE(benchmark_kernel_number_entries));
    printf("  \"BENCHMARK_TOTAL_NUMBER_SCHEDULES\":%lu,\n", (word_t) NODE_STATE(benchmark_kernel_number_schedules));
    printf("  \"BENCHMARK_TCB_\": [\n");
    for (tcb_t *curr = NODE_STATE(ksDebugTCBs); curr != NULL; curr = TCB_PTR_DEBUG_PTR(curr)->tcbDebugNext) {
        printf("    {\n");
        printf("      \"NAME\":\"%s\",\n", TCB_PTR_DEBUG_PTR(curr)->tcbName);
        printf("      \"UTILISATION\":%lu,\n", (word_t) curr->benchmark.utilisation);
        printf("      \"NUMBER_SCHEDULES\":%lu,\n", (word_t) curr->benchmark.number_schedules);
        printf("      \"KERNEL_UTILISATION\":%lu,\n", (word_t) curr->benchmark.kernel_utilisation);
        printf("      \"NUMBER_KERNEL_ENTRIES\":%lu\n", (word_t) curr->benchmark.number_kernel_entries);
        printf("    }");
        if (TCB_PTR_DEBUG_PTR(curr)->tcbDebugNext != NULL) {
            printf(",\n");
        } else {
            printf("\n");
        }
    }
    printf("  ]\n}\n");
    return EXCEPTION_NONE;
}

exception_t handle_SysBenchmarkResetAllThreadsUtilisation(void)
{
    for (tcb_t *curr = NODE_STATE(ksDebugTCBs); curr != NULL; curr = TCB_PTR_DEBUG_PTR(curr)->tcbDebugNext) {
        benchmark_track_reset_utilisation(curr);
    }
    return EXCEPTION_NONE;
}

#endif /* CONFIG_DEBUG_BUILD */
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */
#endif /* CONFIG_ENABLE_BENCHMARKS */
