/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <mode/machine.h>

/* ARM generic timer implementation */
#ifdef CONFIG_KERNEL_MCS
#include <model/statedata.h>
#include <api/types.h>
/** DONT_TRANSLATE **/
static inline ticks_t getCurrentTime(void)
{
    ticks_t time;
    SYSTEM_READ_64(CNT_CT, time);
    return time;
}

/** DONT_TRANSLATE **/
static inline void setDeadline(ticks_t deadline)
{
    /* The timer interrupt condition is met when the counter is greater than or
     * equal to the compare value written here. Thus, writing a values that is
     * in the past is fine.
     */
    SYSTEM_WRITE_64(CNT_CVAL, deadline);
    /* If is is called in the context of an interrupt we have to ensure that the
     * timer deasserts the IRQ before GIC EOIR/DIR. This is sufficient to remove
     * the pending state from the GICR and avoid the timer interrupt happening
     * twice due to the level sensitive configuration.
     * The only case where we are not called from an interrupt is currently from
     * ackDeadlineIRQ() below. The barrier does no matter in this case. But in
     * general, callers of this function expectsthe new timer values to be set
     * when this returns, otherwise unexpected side effects could happen.
     */
    isb();
}

static inline void ackDeadlineIRQ(void)
{
    /* Setting the new value to the maximum practically disables it. It remains
     * unclear why we don't disable it, maybe that is too much overhead given a
     * new timer value is set by the scheduler on kernel exit. The only
     */
    setDeadline(UINT64_MAX);
}
#else /* CONFIG_KERNEL_MCS */
#include <arch/machine/timer.h>
static inline void resetTimer(void)
{
    SYSTEM_WRITE_WORD(CNT_TVAL, TIMER_RELOAD_TICKS);
    /* Ensure that the timer deasserts the IRQ before GIC EOIR/DIR.
     * This is sufficient to remove the pending state from the GICR
     * and avoid the interrupt happening twice because of the level
     * sensitive configuration. */
    isb();
}
#endif /* !CONFIG_KERNEL_MCS */

BOOT_CODE void initGenericTimer(void);

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
static uint64_t read_cntpct(void) UNUSED;
static void save_virt_timer(vcpu_t *vcpu);
static void restore_virt_timer(vcpu_t *vcpu);
#endif /* CONFIG_ARM_HYPERVISOR_SUPPORT */

