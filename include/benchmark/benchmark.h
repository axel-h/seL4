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
#include <sel4/macros.h>
#include <sel4/benchmark_tracepoints_types.h>

#ifdef CONFIG_ENABLE_KERNEL_LOG_BUFFER
extern word_t ksLogIndex = 0;
extern word_t ksLogIndexFinalized = 0;
extern paddr_t ksUserLogBuffer;
#endif /* CONFIG_ENABLE_KERNEL_LOG_BUFFER */

#if defined(CONFIG_DEBUG_BUILD) || defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES)
#define TRACK_KERNEL_ENTRIES 1
extern kernel_entry_t ksKernelEntry;
void benchmark_debug_syscall_start(word_t cptr, word_t msgInfo, word_t syscall);
#endif /* CONFIG_DEBUG_BUILD || CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */

#if defined(CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES) || defined(CONFIG_BENCHMARK_TRACK_UTILISATION)
extern timestamp_t ksTimestampEnter;
void track_kernel_entry(void);
void track_kernel_entry(void);

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
void trace_point_start(word_t id);
#define TRACE_POINT_START(x)  trace_point_start(x)
void trace_point_stop(word_t id);
#define TRACE_POINT_STOP(x)   trace_point_stop(x)
#else /* not CONFIG_MAX_NUM_TRACE_POINTS > 0 */
#define TRACE_POINT_START(x)  SEL4_EMPTY_EXPRESSION()
#define TRACE_POINT_STOP(x)   SEL4_EMPTY_EXPRESSION()
#endif /* [not] CONFIG_MAX_NUM_TRACE_POINTS > 0 */

#enidf /* CONFIG_ENABLE_BENCHMARKS */
