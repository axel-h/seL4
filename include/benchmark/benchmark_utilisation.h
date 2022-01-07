/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION

#include <benchmark/benchmark.h>
#include <sel4/benchmark_utilisation_types.h>
#include <sel4/arch/constants.h>
#include <model/statedata.h>

void benchmark_track_utilisation_dump(void);

void benchmark_track_reset_utilisation(tcb_t *tcb);
/* Calculate and add the utilisation time from when the heir started to run i.e. scheduled
 * and until it's being kicked off
 */
static inline void benchmark_utilisation_switch(tcb_t *heir, tcb_t *next)
{
    timestamp_t timestampEntry = NODE_STATE(trace_kernel_entry);

    /* Add heir thread utilisation */
    if (likely(NODE_STATE(benchmark_log_utilisation_enabled))) {

        /* Check if an overflow occurred while we have been in the kernel */
        if (likely(timestampEntry > heir->benchmark.schedule_start_time)) {

            heir->benchmark.utilisation += (timestampEntry - heir->benchmark.schedule_start_time);

        } else {
#ifdef CONFIG_ARM_ENABLE_PMU_OVERFLOW_INTERRUPT
            heir->benchmark.utilisation += (UINT32_MAX - heir->benchmark.schedule_start_time) + timestampEntry;
            armv_handleOverflowIRQ();
#endif /* CONFIG_ARM_ENABLE_PMU_OVERFLOW_INTERRUPT */
        }

        /* Reset next thread utilisation */
        next->benchmark.schedule_start_time = timestampEntry;
        next->benchmark.number_schedules++;
        NODE_STATE(benchmark_kernel_number_schedules)++;

    }
}

/* Add the time between the last thread got scheduled and when to stop
 * benchmarks
 */
static inline void benchmark_utilisation_finalise(void)
{
    timestamp_t timestampEntry = NODE_STATE(trace_kernel_entry);

    /* Add the time between when NODE_STATE(ksCurThread), and benchmark finalise */
    benchmark_utilisation_switch(NODE_STATE(ksCurThread), NODE_STATE(ksIdleThread));

    NODE_STATE(benchmark_end_time) = timestampEntry;
    NODE_STATE(benchmark_log_utilisation_enabled) = false;
}

#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */
