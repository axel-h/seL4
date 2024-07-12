/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#ifdef CONFIG_KERNEL_MCS

#include <types.h>
#include <mode/util.h>
#include <arch/sbi.h>
#include <arch/machine/hardware.h>

/* The scheduler clock is greater than 1MHz */
#define TICKS_IN_US (TIMER_CLOCK_HZ / (US_IN_MS * MS_IN_S))

static inline CONST time_t getKernelWcetUs(void)
{
    /* Copied from x86_64. Hopefully it's an overestimate here. */
    return  10u;
}

static inline PURE ticks_t usToTicks(time_t us)
{
    return us * TICKS_IN_US;
}

static inline PURE time_t ticksToUs(ticks_t ticks)
{
    return div64(ticks, TICKS_IN_US);
}

/* Get the max. ticks_t value that can be expressed in time_t (time in us). This
 * is the max. value ticksToUs() can be passed without overflowing.
 */
static inline CONST ticks_t getMaxTicksToUs(void)
{
    return UINT64_MAX;
}

/* Get the max. time_t value (time in us) that can be expressed in ticks_t. This
 * is the max. value usToTicks() can be passed without overflowing.
 */
static inline CONST time_t getMaxUsToTicks(void)
{
    return UINT64_MAX / TICKS_IN_US;
}




#endif /* CONFIG_KERNEL_MCS */
