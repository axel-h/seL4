/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Driver for the Samsung Multi Core Timer (MCT)
 */

#include <config.h>
#include <types.h>
#include <plat/machine.h>
#include <linker.h>
#include <drivers/timer/mct.h>

BOOT_CODE void initTimer(void)
{
#ifdef CONFIG_KERNEL_MCS
    const uint32_t wstat_flags = GWSTAT_COMP0_ADD_INC;
    const uint32_t tcon_flags = GTCON_EN | GTCON_COMP0_EN;
#else
    const uint32_t wstat_flags = GWSTAT_COMP0H | GWSTAT_COMP0L | GWSTAT_COMP0_ADD_INC;
    const uint32_t tcon_flags = GTCON_EN | GTCON_COMP0_EN | GTCON_COMP0_AUTOINC;
#endif

    timer_t *mct = mct_get_timer();

    mct_clear_write_status(mct);

    /* Configure the comparator */
#ifdef CONFIG_KERNEL_MCS
    mct->global.comp0_add_inc = 0;
#else
    mct->global.comp0_add_inc = TIMER_RELOAD;
    INC64_ON_2X32(mct->global.comp0h, mct->global.comp0l, TIMER_RELOAD);
#endif

    /* Enable comparator interrupt */
    mct->global.int_en = GINT_COMP0_IRQ;
    /* wait for update */
    while (mct->global.wstat != wstat_flags) {
        /* busy waiting loop */
    }
    /* clear the bits */
    mct->global.wstat = wstat_flags;

    /* enable interrupts */
    mct->global.tcon = tcon_flags;
    /* qait for update */
    while (mct->global.wstat != GWSTAT_TCON) {
        /* busy waiting loop */
    }
    /* clear the bits */
    mct->global.wstat = GWSTAT_TCON;
}
