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

bool_t isValidFaultHanderCap(cap_t handlerCap)
{
    if (!is_cap_endpoint(handlerCap)) {
#ifdef CONFIG_PRINTING
        if (!is_cap_null(handlerCap)) {
            printf("invalid fault handler cap, has type %u\n",
                   cap_get_capType(handlerCap));
        }
#endif /* CONFIG_PRINTING */
        return false;
    }

    if (!cap_endpoint_cap_get_capCanSend(handlerCap) ||
        (!cap_endpoint_cap_get_capCanGrant(handlerCap) &&
         !cap_endpoint_cap_get_capCanGrantReply(handlerCap))) {
        printf("insufficient fault handler cap rights\n");
        return false;
    }

    return true;
}

#ifdef CONFIG_KERNEL_MCS

static int sendFaultIPC(tcb_t *tptr, cap_t handlerCap, bool_t can_donate)
{
    sendIPC(true, false,
            cap_endpoint_cap_get_capEPBadge(handlerCap),
            cap_endpoint_cap_get_capCanGrant(handlerCap),
            cap_endpoint_cap_get_capCanGrantReply(handlerCap),
            can_donate,
            tptr,
            EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));
}


void handleTimeout(tcb_t *tptr)
{
    saveFault(tptr);
    cap_t handlerCap = TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap;
    if (isValidFaultHanderCap(handlerCap) {
        assert(validTimeoutHandler(tptr));
        sendFaultIPC(tptr, handlerCap, false);
        return;
    }

    /* in debug build this is fatal, there must be a timeout handler */
    assert(0);
}

#ifdef CONFIG_PRINTING
    debug_thread_fault(tptr);
#endif /* CONFIG_PRINTING */
    assert(validTimeoutHandler(tptr));
}

#endif /* CONFIG_KERNEL_MCS */

void handleFault(tcb_t *tptr)
{
    saveFault(tptr);

#ifdef CONFIG_KERNEL_MCS
    cap_t handlerCap = TCB_PTR_CTE_PTR(tptr, tcbFaultHandler)->cap;
    if (isValidFaultHanderCap(handlerCap) {
        bool_t can_donate = (tptr->tcbSchedContext != NULL);
        sendFaultIPC(tptr, handlerCap, can_donate);
        return
    }
#else /* not CONFIG_KERNEL_MCS */
    lookupCap_ret_t lu_ret = lookupCap(tptr, tptr->tcbFaultHandler);
    if (lu_ret.status == EXCEPTION_NONE) {
        cap_t handlerCap = lu_ret.cap;
        if (isValidFaultHanderCap(handlerCap)) {
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
