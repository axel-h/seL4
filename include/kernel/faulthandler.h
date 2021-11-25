/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <object.h>

void handleFault(tcb_t *tptr);

#ifdef CONFIG_KERNEL_MCS

static inline bool_t validTimeoutHandler(tcb_t *tcb)
{
    cte_t *cte = TCB_PTR_CTE_PTR(tcb, tcbTimeoutHandler);
    word_t cap_type = cap_get_capType(cte->cap);
    return (cap_endpoint_cap == cap_type);
}

void handleTimeout(tcb_t *tptr);

#endif /* CONFIG_KERNEL_MCS */
