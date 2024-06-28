/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4/macros.h>
#include <sel4/simple_types.h>
#include <sel4/sel4_arch/types.h>

typedef seL4_CPtr seL4_ACME_Page;
typedef seL4_CPtr seL4_ACME_PageTable;
typedef seL4_CPtr seL4_ACME_ASIDControl;
typedef seL4_CPtr seL4_ACME_ASIDPool;
typedef seL4_CPtr seL4_ACME_VCPU;
typedef seL4_CPtr seL4_ACME_IOSpace;
typedef seL4_CPtr seL4_ACME_IOPageTable;

#define seL4_EndpointBits     4
/* User context as used by seL4_TCB_ReadRegisters / seL4_TCB_WriteRegisters */

typedef struct seL4_UserContext_ {
    seL4_Word pc;
    seL4_Word sp;
    seL4_Word tp;
    seL4_Word ra;
    seL4_Word r0;
    seL4_Word r1;
    seL4_Word r2;
    seL4_Word r3;
} seL4_UserContext;

typedef enum {
    seL4_ACME_ExecuteNever = 0x1,
    seL4_ACME_Default_VMAttributes = 0,
    SEL4_FORCE_LONG_ENUM(seL4_ACME_VMAttributes)
} seL4_ACME_VMAttributes;
