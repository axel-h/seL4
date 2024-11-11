/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

/* The value for the max number of free memory region is basically an arbitrary
 * choice. We could make the macro calculate the exact number, but just picking
 * a value will also do for now. Increase this value if the boot fails.
 */
#define MAX_NUM_FREEMEM_REG 16

#define NUM_RESERVED_REGIONS \
    ( \
        CONFIG_MAX_NUM_IOAPIC \
        + MAX_NUM_DRHU \
        + 1 /* APIC */ \
        + 1 /* MSI region */ \
        + 1 /* user image */ \
    )
