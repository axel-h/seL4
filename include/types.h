/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <basic_types.h>
#include <api/types.h>
#include <object/structures.h>
#include <machine_util.h>

typedef struct pde_range {
    pde_t *base;
    word_t length;
} pde_range_t;

typedef struct pte_range {
    pte_t *base;
    word_t length;
} pte_range_t;

typedef cte_t *cte_ptr_t;

typedef struct extra_caps {
    cte_ptr_t excaprefs[seL4_MsgMaxExtraCaps];
} extra_caps_t;
