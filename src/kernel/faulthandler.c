/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <api/failures.h>
#include <api/debug.h>
#include <kernel/cspace.h>
#include <kernel/faulthandler.h>
#include <kernel/thread.h>
#include <arch/machine.h>


static void saveFault(tcb_t *tptr)
{
    seL4_Fault_t fault = current_fault;
    lookup_fault_t lookup_fault = current_lookup_fault;

    tptr->tcbFault = fault;
    if (seL4_Fault_get_seL4_FaultType(fault) == seL4_Fault_CapFault) {
        tptr->tcbLookupFailure = lookup_fault;
    }
}

#ifdef CONFIG_KERNEL_MCS

static bool_t sendFaultIPC(tcb_t *tptr, cap_t handlerCap, bool_t can_donate)
{
    if (cap_get_capType(handlerCap) == cap_endpoint_cap) {
        assert(cap_endpoint_cap_get_capCanSend(handlerCap));
        assert(cap_endpoint_cap_get_capCanGrant(handlerCap) ||
               cap_endpoint_cap_get_capCanGrantReply(handlerCap));

        sendIPC(true, false,
                cap_endpoint_cap_get_capEPBadge(handlerCap),
                cap_endpoint_cap_get_capCanGrant(handlerCap),
                cap_endpoint_cap_get_capCanGrantReply(handlerCap),
                can_donate, tptr,
                EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));

        return true;
    } else {
        assert(cap_get_capType(handlerCap) == cap_null_cap);
        return false;
    }
}

void handleTimeout(tcb_t *tptr)
{
    saveFault(tptr);
    assert(validTimeoutHandler(tptr));
    sendFaultIPC(tptr, TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap, false);
}

#endif /* CONFIG_KERNEL_MCS */

void handleFault(tcb_t *tptr)
{
    saveFault(tptr);

#ifdef CONFIG_KERNEL_MCS
    if (sendFaultIPC(tptr, TCB_PTR_CTE_PTR(tptr, tcbFaultHandler)->cap,
                     tptr->tcbSchedContext != NULL)) {
            return;
    }
#else /* not CONFIG_KERNEL_MCS */
    lookupCap_ret_t lu_ret = lookupCap(tptr, tptr->tcbFaultHandler);
    if (lu_ret.status == EXCEPTION_NONE) {
        cap_t handlerCap = lu_ret.cap;
        if (cap_get_capType(handlerCap) == cap_endpoint_cap &&
            cap_endpoint_cap_get_capCanSend(handlerCap) &&
            (cap_endpoint_cap_get_capCanGrant(handlerCap) ||
             cap_endpoint_cap_get_capCanGrantReply(handlerCap))) {
            sendIPC(true, true,
                    cap_endpoint_cap_get_capEPBadge(handlerCap),
                    cap_endpoint_cap_get_capCanGrant(handlerCap), true, tptr,
                    EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));
            return;
        }
    }
#endif /* [not] CONFIG_KERNEL_MCS */

    /* Seems there is no fault handler, log the fault and suspend the thread. */
#ifdef CONFIG_PRINTING
    debug_thread_fault(tptr);
#endif /* CONFIG_PRINTING */
    setThreadState(tptr, ThreadState_Inactive);
}
