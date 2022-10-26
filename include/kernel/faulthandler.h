/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <object.h>

#ifdef CONFIG_KERNEL_MCS
static inline bool_t validTimeoutHandler(tcb_t *tptr)
{
    return cap_get_capType(TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap) == cap_endpoint_cap;
}

void handleTimeout(tcb_t *tptr);
#endif
void handleFault(tcb_t *tptr);

