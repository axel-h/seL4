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

#ifdef CONFIG_PRINTING

static void print_fault(seL4_Fault_t f)
{
    switch (seL4_Fault_get_seL4_FaultType(f)) {
    case seL4_Fault_NullFault:
        printf("null fault");
        break;
    case seL4_Fault_CapFault:
        printf("cap fault in %s phase at address %p",
               seL4_Fault_CapFault_get_inReceivePhase(f) ? "receive" : "send",
               (void *)seL4_Fault_CapFault_get_address(f));
        break;
    case seL4_Fault_VMFault:
        printf("vm fault on %s at address %p with status %p",
               seL4_Fault_VMFault_get_instructionFault(f) ? "code" : "data",
               (void *)seL4_Fault_VMFault_get_address(f),
               (void *)seL4_Fault_VMFault_get_FSR(f));
        break;
    case seL4_Fault_UnknownSyscall:
        printf("unknown syscall %p",
               (void *)seL4_Fault_UnknownSyscall_get_syscallNumber(f));
        break;
    case seL4_Fault_UserException:
        printf("user exception %p code %p",
               (void *)seL4_Fault_UserException_get_number(f),
               (void *)seL4_Fault_UserException_get_code(f));
        break;
#ifdef CONFIG_KERNEL_MCS
    case seL4_Fault_Timeout:
        printf("Timeout fault for 0x%x\n", (unsigned int) seL4_Fault_Timeout_get_badge(f));
        break;
#endif
    default:
        printf("unknown fault");
        break;
    }
}

static void printFaultHandlerError(tcb_t *tptr, seL4_Fault_t fault)
{
#ifdef CONFIG_KERNEL_MCS
    printf("Found thread has no fault handler while trying to handle:\n");
#else // not CONFIG_KERNEL_MCS
    printf("Caught ");
    print_fault(current_fault);
    printf("\nwhile trying to handle:\n");
#endif
#ifdef CONFIG_DEBUG_BUILD
    print_fault(fault);
    printf("\nin thread %p \"%s\" ", tptr, TCB_PTR_DEBUG_PTR(tptr)->tcbName);
#endif
    printf("at address %p\n", (void *)getRestartPC(tptr));
    printf("With stack:\n");
    Arch_userStackTrace(tptr);
}

#endif /* CONFIG_PRINTING */

#ifdef CONFIG_KERNEL_MCS
void handleTimeout(tcb_t *tptr)
{
    assert(validTimeoutHandler(tptr));
    tptr->tcbFault = current_fault;
    sendFaultIPC(tptr, TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap, false);
}

bool_t sendFaultIPC(tcb_t *tptr, cap_t handlerCap, bool_t can_donate)
{
    if (cap_get_capType(handlerCap) == cap_endpoint_cap) {
        assert(isValidFaultHandlerEp(handlerCap));
        sendIPC(true, /* blocking */
                false, /* don't do a call */
                cap_endpoint_cap_get_capEPBadge(handlerCap),
                cap_endpoint_cap_get_capCanGrant(handlerCap),
                cap_endpoint_cap_get_capCanGrantReply(handlerCap),
                can_donate,
                tptr,
                EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));

        return true;
    } else {
        assert(cap_get_capType(handlerCap) == cap_null_cap);
        return false;
    }
}
#else

exception_t sendFaultIPC(tcb_t *tptr)
{
    cptr_t handlerCPtr = tptr->tcbFaultHandler;
    lookupCap_ret_t lu_ret = lookupCap(tptr, handlerCPtr);
    if (lu_ret.status != EXCEPTION_NONE) {
        /* lookup failed */
        // ToDo: Seems we are setting this just for the error printing, as
        //       nobody else cares about this practically?
        //       Also, why don't we set current_lookup_fault here also?
        current_fault = seL4_Fault_CapFault_new(handlerCPtr, false);
        return EXCEPTION_FAULT;
    }

    cap_t handlerCap = lu_ret.cap;
    if (!isValidFaultHandlerEp(handlerCap)) {
        /* not and endpoint or invalid endpoint rights */
        // ToDo: Seems we are setting this just for the error printing, as
        //       nobody else cares about this practically?
        current_fault = seL4_Fault_CapFault_new(handlerCPtr, false);
        current_lookup_fault = lookup_fault_missing_capability_new(0);
        return EXCEPTION_FAULT;
    }

    sendIPC(true, /* blocking */
            true, /* do a call */
            cap_endpoint_cap_get_capEPBadge(handlerCap),
            cap_endpoint_cap_get_capCanGrant(handlerCap),
            true, /* can grant reply */
            tptr,
            EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));

    return EXCEPTION_NONE;
}
#endif

void handleFault(tcb_t *tptr)
{
    seL4_Fault_t fault = current_fault;
    tptr->tcbFault = fault;
    if (seL4_Fault_get_seL4_FaultType(fault) == seL4_Fault_CapFault) {
        tptr->tcbLookupFailure = current_lookup_fault;
    }

    bool_t dispatchOk;
#ifdef CONFIG_KERNEL_MCS
    dispatchOk = sendFaultIPC(tptr,
                              TCB_PTR_CTE_PTR(tptr, tcbFaultHandler)->cap,
                              tptr->tcbSchedContext != NULL);
#else
    dispatchOk = (EXCEPTION_NONE == sendFaultIPC(tptr));
#endif

    if (dispatchOk) {
        /* The fault handler will hande the thread's fault */
        return;
    }

    /*
     * There is no fault handler that could be notified about the fault, log
     * some fault details and suspend the faulting thread.
     */
#ifdef CONFIG_PRINTING
    printFaultHandlerError(tptr, fault);
#endif

    setThreadState(tptr, ThreadState_Inactive);
}
