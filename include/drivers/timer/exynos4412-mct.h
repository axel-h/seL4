/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <drivers/timer/mct.h>

static inline void resetTimer(void)
{
    timer_t *mct = mct_get_timer();
    mct_reset(mct);
}

#ifdef CONFIG_KERNEL_MCS
/** DONT_TRANSLATE **/
static inline ticks_t getCurrentTime(void)
{
    timer_t *mct = mct_get_timer();

    /* Reading the counter must handle overflows on the low part. There is no
     * need to loop here, because this is kernel code and thus can't get
     * interrupted.
     */
    uint32_t hi = mct->global.cnth;
    uint32_t lo = mct->global.cntl;
    uint32_t hi2 = mct->global.cnth;
    if (hi != hi2) {
        lo = mct->global.cntl;
    }
    return MAKE_U64(hi, lo);
}

/** DONT_TRANSLATE **/
static inline void setDeadline(ticks_t deadline)
{
    timer_t *mct = mct_get_timer();

    /*
     * After writing a value to a comp register a bit in the wstat
     * register is asserted. A write of 1 clears this bit.
     */
    mct->global.comp0h = (uint32_t)(deadline >> 32u);
    while (!(mct->global.wstat & GWSTAT_COMP0H));
    mct->global.wstat |= GWSTAT_COMP0H;

    mct->global.comp0l = (uint32_t) deadline;

    while (!(mct->global.wstat & GWSTAT_COMP0L));
    mct->global.wstat |= GWSTAT_COMP0L;
}

static inline void ackDeadlineIRQ(void)
{
    timer_t *mct = mct_get_timer();

    /* ack everything */
    mct_reset(mct);
}

#endif


