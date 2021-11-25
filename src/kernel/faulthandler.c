/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <api/failures.h>
#include <kernel/cspace.h>
#include <kernel/faulthandler.h>
#include <kernel/thread.h>
#include <machine/io.h>
#include <arch/machine.h>

#ifdef CONFIG_KERNEL_MCS

static bool_t sendFaultIPC(tcb_t *tptr, cap_t handlerCap, bool_t can_donate,
                           seL4_Fault_t fault)
{
    if (cap_get_capType(handlerCap) == cap_endpoint_cap) {
        assert(cap_endpoint_cap_get_capCanSend(handlerCap));
        assert(cap_endpoint_cap_get_capCanGrant(handlerCap) ||
               cap_endpoint_cap_get_capCanGrantReply(handlerCap));

        tptr->tcbFault = fault;
        sendIPC(true, false,
                cap_endpoint_cap_get_capEPBadge(handlerCap),
                cap_endpoint_cap_get_capCanGrant(handlerCap),
                cap_endpoint_cap_get_capCanGrantReply(handlerCap),
                can_donate, tptr,
                EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));

        return true;
    }

    assert(cap_get_capType(handlerCap) == cap_null_cap);
    return false;
}

void handleTimeout(tcb_t *tptr)
{
    assert(validTimeoutHandler(tptr));
    seL4_Fault_t fault = current_fault;
    cap_t handlerCap = TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap;
    (void)sendFaultIPC(tptr, handlerCap, false, fault);
}

#endif /* CONFIG_KERNEL_MCS */

void handleFault(tcb_t *tptr)
{
    seL4_Fault_t fault = current_fault;

#ifdef CONFIG_KERNEL_MCS

    if (sendFaultIPC(tptr,
                     TCB_PTR_CTE_PTR(tptr, tcbFaultHandler)->cap,
                     (tptr->tcbSchedContext != NULL),
                     fault)) {
        return; /* fault handler will take care of the fault */
    }

#else /* not CONFIG_KERNEL_MCS */

    lookup_fault_t original_lookup_fault = current_lookup_fault;
    cptr_t handlerCPtr = tptr->tcbFaultHandler;
    lookupCap_ret_t lu_ret = lookupCap(tptr, handlerCPtr);
    exception_t status = lu_ret.status;
    if (EXCEPTION_NONE == status) {
        cap_t handlerCap = lu_ret.cap;
        if (cap_get_capType(handlerCap) == cap_endpoint_cap &&
            cap_endpoint_cap_get_capCanSend(handlerCap) &&
            (cap_endpoint_cap_get_capCanGrant(handlerCap) ||
             cap_endpoint_cap_get_capCanGrantReply(handlerCap))) {
            tptr->tcbFault = fault;
            if (seL4_Fault_get_seL4_FaultType(fault) == seL4_Fault_CapFault) {
                tptr->tcbLookupFailure = original_lookup_fault;
            }
            sendIPC(true, true,
                    cap_endpoint_cap_get_capEPBadge(handlerCap),
                    cap_endpoint_cap_get_capCanGrant(handlerCap), true, tptr,
                    EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));
            return; /* fault handler will take care of the fault */
        }

        current_lookup_fault = lookup_fault_missing_capability_new(0);
    }

#endif /* [not] CONFIG_KERNEL_MCS */

    /* The fault could not be reported to a userland fault handler, so nobody
     * can handle (or even fix) it. We have no choice but suspending the thread
     * that caused the fault. That may cause more problems in userland and
     * eventually make the system deadlock or even crash, but there is nothing
     * we can do from the kernel's point of view. If printing is enabled,
     * logging some details may help doing a post-mortem analysis.
     */

#ifdef CONFIG_PRINTING
    print_unhandled_fault(tptr,
                          fault,
                          config_ternary(CONFIG_KERNEL_MCS,
                                         EXCEPTION_NONE,
                                         status));
#endif /* CONFIG_PRINTING */

    setThreadState(tptr, ThreadState_Inactive);
}
