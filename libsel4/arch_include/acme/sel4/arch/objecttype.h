/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>

typedef enum _object {
    seL4_ACME_4K_Page = seL4_ModeObjectTypeCount,
    seL4_ACME_Mega_Page,
    seL4_ACME_PageTableObject,
    seL4_ObjectTypeCount
} seL4_ArchObjectType;

typedef seL4_Word object_t;
