/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <object.h>

static inline bool_t is_cap_null(cap_t cap)
{
    return (cap_get_capType(cap) == cap_null_cap);
}

static inline bool_t is_cap_endpoint(cap_t cap)
{
    return (cap_get_capType(cap) == cap_endpoint_cap);
}

bool_t isValidFaultHanderCap(cap_t handlerCap);

#ifdef CONFIG_KERNEL_MCS

static inline bool_t validTimeoutHandler(tcb_t *tptr)
{
    cap_t handler_cap = TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap;
    return isValidFaultHanderCap(handler_cap);
}

void handleTimeout(tcb_t *tptr);
#endif
void handleFault(tcb_t *tptr);
