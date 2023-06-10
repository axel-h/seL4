/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <plat/machine/devices_gen.h>

#define MAX_NUM_FREEMEM_REG \
    ( \
        ARRAY_SIZE(avail_p_regs) /* from the auto-generated code */ \
        + 1 /* ASID area */ \
        + 1 /* allow kernel to release its own boot data region */ \
        + 1 /* possible gap between ELF images and rootserver objects, see arm/arch_init_freemem */ \
    )

#define NUM_RESERVED_REGIONS \
    ( \
        NUM_KERNEL_DEVICE_FRAMES \
        + 1 /* ASID area */ \
        + 1 /* kernel image */ \
        + 1 /* usage image */ \
        + 1 /* DTB from bootloader */ \
    )
