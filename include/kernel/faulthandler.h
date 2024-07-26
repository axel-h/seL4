/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <types.h>
#include <object.h>

/* ToDo: seems there is no dedicated value */
#define NO_LOOKUP_FAULT lookup_fault_missing_capability_new(0)

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

bool_t tryRaisingTimeoutFault(tcb_t *tptr, word_t scBadge);
#endif
void handleFault(tcb_t *tptr);

