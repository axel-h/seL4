/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <arch/machine/timer.h>

#ifdef CONFIG_KERNEL_MCS
#include <types.h>
#include <arch/linker.h>

/* Read the current time from the timer. */
/** MODIFIES: [*] */
static inline ticks_t getCurrentTime(void);
/* Set the next deadline irq - deadline is absolute and may be slightly in
   the past. If it is set in the past, we expect an interrupt to be raised
   immediately after we leave the kernel. */
/** MODIFIES: [*] */
static inline void setDeadline(ticks_t deadline);
/* ack previous deadline irq */
/** MODIFIES: [*] */
static inline void ackDeadlineIRQ(void);

static inline CONST time_t getKernelWcetUs(void)
{
    /* On ARM and RISC-V the CMake macro declare_default_headers() allows
     * platforms to set custom values for CONFIGURE_KERNEL_WCET. On x86 there
     * is no such mechanism because there is the 'pc99' platform only, so a
     * custom value could be defined in 'include/plat/pc99/plat/machine.h' or
     * 'include/arch/x86/arch/machine/timer.h'
     */
#ifdef CONFIGURE_KERNEL_WCET
    return CONFIGURE_KERNEL_WCET;
#else
    /*
     * Using 10 us turned out to be a good default value, but it seems this has
     * been copy/pasted ever since. At 1 GHz this corresponds to 10.000 cylces,
     * which seems very far on safe side for modern platforms. But actuallay,
     * it would be good to have a better explanation how to practically check
     * how good this value is in a specific platform and a reasonable system
     * configuration.
     */
    return 10;
#endif
}

/* get the expected wcet of the kernel for this platform */
static PURE inline ticks_t getKernelWcetTicks(void)
{
    return usToTicks(getKernelWcetUs());
}
#else /* CONFIG_KERNEL_MCS */
static inline void resetTimer(void);
#endif /* !CONFIG_KERNEL_MCS */

