/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <object.h>

static bool_t isValidFaultHandlerEp(cap_t cap)
{
    return ((cap_get_capType(cap) == cap_endpoint_cap) &&
            cap_endpoint_cap_get_capCanSend(cap) &&
            (cap_endpoint_cap_get_capCanGrant(cap) ||
             cap_endpoint_cap_get_capCanGrantReply(cap)));
}

#ifdef CONFIG_KERNEL_MCS

static bool_t isValidFaultHandlerEpOrNull(cap_t cap)
{
    return ((cap_get_capType(cap) == cap_null_cap) || isValidFaultHandlerEp(cap));
}

static inline bool_t validTimeoutHandler(tcb_t *tptr)
{
    cap_t handlerCap = TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap;
    return isValidFaultHandlerEp(handlerCap);
}

void handleTimeout(tcb_t *tptr);
#endif
void handleFault(tcb_t *tptr);

