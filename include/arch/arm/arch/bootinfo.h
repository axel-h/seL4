/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>

#if defined(CONFIG_ARCH_AARCH32)

#include <plat/machine/devices_gen.h>
#include <kernel/vspace.h>

#define MAX_NUM_FREEMEM_REG \
    ( \
        ARRAY_SIZE(avail_p_regs) /* from the auto-generated code */ \
        + MODE_RESERVED \
        + 1 /* allow kernel to release its own boot data region */ \
        + 1 /* possible gap between ELF images and rootserver objects, see arm/arch_init_freemem */ \
    )

#define NUM_RESERVED_REGIONS \
    ( \
        NUM_KERNEL_DEVICE_FRAMES \
        + MODE_RESERVED \
        + 1 /* kernel image */ \
        + 1 /* usage image */ \
        + 1 /* DTB from bootloader */ \
    )

#elif defined(CONFIG_ARCH_AARCH64)

/* The value for the max number of free memory region is basically an arbitrary
 * choice. We could make the macro calculate the exact number, but just picking
 * a value will also do for now. Increase this value if the boot fails.
 */
#define MAX_NUM_FREEMEM_REG 16

#define NUM_RESERVED_REGIONS \
    ( \
        NUM_KERNEL_DEVICE_FRAMES \
        + 1 /* kernel image */ \
        + 1 /* usage image */ \
        + 1 /* DTB from bootloader */ \
    )

#else
#error "unknown ARM architecture"
#endif
