/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Driver for the Samsung Multi Core Timer (MCT)
 */

#include <config.h>
#include <types.h>
#include <machine/io.h>
#include <kernel/vspace.h>
#include <arch/machine.h>
#include <arch/kernel/vspace.h>
#include <plat/machine.h>
#include <linker.h>
#include <plat/machine/devices_gen.h>
#include <plat/machine/hardware.h>
#include <drivers/timer/arm_generic.h>
#include <drivers/timer/mct.h>

BOOT_CODE void initTimer(void)
{
    timer_t *mct = mct_get_timer();

    mct_clear_write_status(mct);

    /* use the arm generic timer, backed by the mct */
    /* enable the timer */
    mct->global.tcon = GTCON_EN;
    while (mct->global.wstat != GWSTAT_TCON) {
        /* busy loop */
    }
    /* clear the bit */
    mct->global.wstat = GWSTAT_TCON;

    initGenericTimer();
}

