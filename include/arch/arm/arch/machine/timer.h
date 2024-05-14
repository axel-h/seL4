/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <stdint.h>

#ifdef CONFIG_KERNEL_MCS

#include <types.h>
#include <util.h>
#include <mode/util.h>

/*
 * We can use MHz if the timer clock is a straigth MHz value, otherwise the
 * calculations must use KHz to avoid rounding errors
 */
#define USE_KHZ (TIMER_CLOCK_HZ % HZ_IN_MHZ > 0)

#define TIMER_CLOCK_KHZ (TIMER_CLOCK_HZ / HZ_IN_KHZ)
#define TIMER_CLOCK_MHZ (TIMER_CLOCK_HZ / HZ_IN_MHZ)

#else /* not CONFIG_KERNEL_MCS */

#include <mode/machine/timer.h>
#include <plat/machine/hardware.h>

/* convert to khz first to avoid overflow */
#define TICKS_PER_MS (TIMER_CLOCK_HZ / HZ_IN_KHZ)
/* but multiply by timer tick ms */
#define TIMER_RELOAD_TICKS    (TICKS_PER_MS * CONFIG_TIMER_TICK_MS)
#if (TIMER_RELOAD_TICKS >= UINTPTR_MAX)
#error "Timer reload too high"
#endif

#endif /* [not] CONFIG_KERNEL_MCS */

void initTimer(void);

#ifdef CONFIG_KERNEL_MCS

/* Get the max. time_t value (time in us) that can be expressed in ticks_t. This
 * is the max. value usToTicks() can be passed without overflowing.
 */
static inline CONST time_t getMaxUsToTicks(void)
{
#if USE_KHZ
    return _as_time_t(UINT64_MAX / TIMER_CLOCK_KHZ);
#else
    return _as_time_t(UINT64_MAX / TIMER_CLOCK_MHZ);
#endif
}

static inline CONST ticks_t usToTicks(time_t us)
{
#if USE_KHZ
    /* reciprocal division overflows too quickly for dividing by KHZ_IN_MHZ.
     * This operation isn't used frequently or on many platforms, so use manual
     * division here */
    return div64(_from_time_t(us) * TIMER_CLOCK_KHZ, KHZ_IN_MHZ);
#else
    return _from_time_t(us) * TIMER_CLOCK_MHZ;
#endif
}

static inline CONST ticks_t getTimerPrecision(void)
{
    return usToTicks(_as_time_t(TIMER_PRECISION)) + TIMER_OVERHEAD_TICKS;
}

# endif /* CONFIG_KERNEL_MCS */
