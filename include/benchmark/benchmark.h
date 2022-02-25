/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <util.h>

#ifdef CONFIG_ENABLE_BENCHMARKS

#include <types.h>
#include <arch/benchmark.h>
#include <machine/io.h>
#include <sel4/arch/constants.h>
#include <arch/machine/hardware.h>
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
