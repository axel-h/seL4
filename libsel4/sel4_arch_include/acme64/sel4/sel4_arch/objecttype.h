/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>

typedef enum _mode_object {
    seL4_ACME_Giga_Page = seL4_NonArchObjectTypeCount,
    seL4_ModeObjectTypeCount
} seL4_ModeObjectType;
