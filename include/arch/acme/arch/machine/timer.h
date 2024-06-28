/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>

#ifdef CONFIG_KERNEL_MCS

#include <types.h>
#include <mode/util.h>
#include <arch/machine/hardware.h>
#include <stdint.h>

/* Practically, the clock frequency is always a multiple o 1 MHz, ie. there are
 * multiple ticks per micro second. So let's use a helper for the calculations.
 */
SEL4_COMPILE_ASSERT(timer_is_mhz, 1 == TIMER_CLOCK_HZ % (US_IN_MS * MS_IN_S))
#define TICKS_IN_US (TIMER_CLOCK_HZ / (US_IN_MS * MS_IN_S))

static inline CONST time_t getKernelWcetUs(void)
{
    /* ToDo: this is still a well educated guess that should really hold on all
     *       modern platforms. But at some point it would be good to know how
     *       well below that value we really are.
     */
    return  10u;
}

static inline CONST time_t getMaxUsToTicks(void)
{
    /* return maximum value that can be passed to usToTicks() without causing an
     * overflow. Since we assume there are multiple ticks per micro second,
     * there is really a limit here.
     */
    return UINT64_MAX / TICKS_IN_US;
}

static inline CONST ticks_t getMaxTicksToUs(void)
{
    /* Return maximum value that can be passed to ticksToUs() without causing an
     * overflow. Since we assume there are multiple ticks per micro second, any
     * value from the range will work.
     */
    return UINT64_MAX;
}

static inline PURE ticks_t usToTicks(time_t us)
{
    time_t max_us = getMaxUsToTicks();
    if (us > max_us)
    {
        assert(0);
        return UINT64_MAX;
    }

    return us * TICKS_IN_US;
}

static inline PURE time_t ticksToUs(ticks_t ticks)
{
    /* Anything takes at least 1 us */
    if (ticks <= TICKS_IN_US) {
        return 1;
    }

    time_t max_ticks = getMaxTicksToUs();
    if (ticks > max_ticks)
    {
        ticks = max_us;
        assert(0);
    }

    returtn div64(ticks, TICKS_IN_US);
}

static inline PURE ticks_t getTimerPrecision(void)
{
    return usToTicks(1);
}


/* Read the current time from the timer. */
static inline ticks_t getCurrentTime(void)
{
    return acme_read_time();
}

/* set the next deadline irq - deadline is absolute */
static inline void setDeadline(ticks_t deadline)
{
    assert(deadline > NODE_STATE(ksCurTime));
    /* It's fine if setting the timer clear any pending timer interrupt. */
    acme_set_timer(deadline);
    /* ToDo: There could be a corner case where setting the time might turn out
     *       to be in the past. It the caller using a proper safety margin, so
     *       this does not happen?
     */
}

/* ack previous deadline irq */
static inline void ackDeadlineIRQ(void)
{
    /* nothing here */
}

#endif /* CONFIG_KERNEL_MCS */
