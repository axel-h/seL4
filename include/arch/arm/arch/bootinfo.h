/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <plat/machine/devices_gen.h>

enum {
    FREEMEM_REG_BOOT_DATA, /* allow kernel to release its own boot data region */
    FREEMEM_REG_GAP, /* possible gap between ELF images and rootserver objects */
#ifdef CONFIG_ARCH_AARCH32
    FREEMEM_REG_HW_ASID, /* hw_asid_region from vspace.h */
#endif
    /* --- */
    NUM_FREEMEM_REGS,
    /* --- */
    MAX_NUM_FREEMEM_REG = ARRAY_SIZE(avail_p_regs) + NUM_FREEMEM_REGS
};

enum {
    RESERVED_REG_KERNEL,
    RESERVED_REG_DEVICE_TREE_BINARY,
    RESERVED_REG_USER_IMAGE,
#ifdef CONFIG_ARCH_AARCH32
    RESERVED_REG_HW_ASID,
#endif
    /* --- */
    NUM_RESERVED_REGIONS
};

/* The maximum number of reserved regions is:
 * -  each free memory region (MAX_NUM_FREEMEM_REG)
 * -  each kernel frame (NUM_KERNEL_DEVICE_FRAMES, there might be none)
 * -  each region reserved by the boot code (NUM_RESERVED_REGIONS)
 */
#define MAX_NUM_RESV_REG (MAX_NUM_FREEMEM_REG + NUM_KERNEL_DEVICE_FRAMES + \
                          NUM_RESERVED_REGIONS)
