/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <util.h>
#include <api/failures.h>
#include <api/debug.h>
#include <kernel/cspace.h>
#include <kernel/faulthandler.h>
#include <kernel/thread.h>
#include <arch/machine.h>

#ifdef CONFIG_PRINTING

#if CONFIG_WORD_SIZE == 32
#define PRI_reg "%08"SEL4_PRIx_word
#define PRI_reg_param(r)    (r)
#elif CONFIG_WORD_SIZE == 64
#define PRI_reg "%08"PRIx32"'%08"PRIx32
#define PRI_reg_param(r)    (uint32_t)((r) >> 32), (uint32_t)(r)
#else
#error "unsupported CONFIG_WORD_SIZE"
#endif

static void print_fault(seL4_Fault_t f)
{
    const char *name = config_ternary(CONFIG_DEBUG_BUILD,
                                      TCB_PTR_DEBUG_PTR(tptr)->tcbName,
                                      null);

    printf("\n\n"
           "####\n"
           "####  FAULT at PC=%p in thread %p%s%s%s\n"
           "####\n",
           (void *)getRestartPC(tptr), tptr,
           name ? " (" : "", name, name ? ")" : "");


    seL4_Fault_t fault = tptr->tcbFault;
    word_t fault_type = seL4_Fault_get_seL4_FaultType(fault);
    printf("Cause: "); /* message follows here */
    switch (fault_type) {
    case seL4_Fault_NullFault:
        printf("null fault");
        break;
    case seL4_Fault_CapFault:
        /* tptr->tcbLookupFailure is also set */
        printf("cap fault in %s phase at address %p",
               seL4_Fault_CapFault_get_inReceivePhase(fault) ? "receive" : "send",
               (void *)seL4_Fault_CapFault_get_address(fault));
        break;
    case seL4_Fault_VMFault:
        printf("vm fault on %s at address %p with status %p",
               seL4_Fault_VMFault_get_instructionFault(fault) ? "code" : "data",
               (void *)seL4_Fault_VMFault_get_address(fault),
               (void *)seL4_Fault_VMFault_get_FSR(fault));
        break;
    case seL4_Fault_UnknownSyscall:
        printf("unknown syscall %p",
               (void *)seL4_Fault_UnknownSyscall_get_syscallNumber(fault));
        break;
    case seL4_Fault_UserException:
        printf("user exception %p code %p",
               (void *)seL4_Fault_UserException_get_number(fault),
               (void *)seL4_Fault_UserException_get_code(fault));
        break;
#ifdef CONFIG_KERNEL_MCS
    case seL4_Fault_Timeout:
        printf("Timeout fault for 0x%x", (unsigned int)seL4_Fault_Timeout_get_badge(fault));
        break;
#endif
    default:
        printf("unknown fault 0x%x", (unsigned int)fault_type);
        break;
    }

    printf("\n");

    /* Thread registers are printed in debug builds only. */
#ifdef CONFIG_DEBUG_BUILD
    printf("Registers:\n");
    user_context_t *user_ctx = &(tptr->tcbArch.tcbContext);
    /* Dynamically adapt spaces to max register name len per column. */
    unsigned int max_reg_name_len[2] = {0};
    for (unsigned int i = 0; i < ARRAY_SIZE(user_ctx->registers); i++) {
        int col = i & 1;
        unsigned int l = strnlen(register_names[i], 20);
        if (max_reg_name_len[col] < l) {
            max_reg_name_len[col] = l;
        }
    }
    const unsigned int num_regs = ARRAY_SIZE(user_ctx->registers);
    for (unsigned int i = 0; i < num_regs; i++) {
        int col = i & 1;
        bool_t is_last = (num_regs == i + 1);
        word_t reg = user_ctx->registers[i];
        printf("%*s: 0x"PRI_reg"%s",
               max_reg_name_len[col] + 2, register_names[i], PRI_reg_param(reg),
               (col || is_last) ? "\n" : "");
    }
    printf("\nStack:\n");
    Arch_userStackTrace(tptr);
#endif /* CONFIG_DEBUG_BUILD */

    printf("\nThread suspended, no userland fault handler\n");
}

#endif /* CONFIG_PRINTING */

#ifdef CONFIG_KERNEL_MCS
static void sendFaultIPC(cap_t handlerCap, tcb_t *tptr, bool_t can_donate)
{
    sendIPC(true, /* blocking */
            false, /* don't do a call */
            cap_endpoint_cap_get_capEPBadge(handlerCap),
            cap_endpoint_cap_get_capCanGrant(handlerCap),
            cap_endpoint_cap_get_capCanGrantReply(handlerCap),
            can_donate,
            tptr,
            EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));
}

bool_t tryRaisingTimeoutFault(tcb_t *tptr, word_t scBadge)
{
    seL4_Fault_t fault = seL4_Fault_Timeout_new(scBadge);
    current_fault = fault; // ToDo: do we really need this any longer?
    tptr->tcbFault = fault;

    cap_t handlerCap = TCB_PTR_CTE_PTR(tptr, tcbTimeoutHandler)->cap;
    if (!isValidFaultHandlerEp(handlerCap))
    {
        assert(cap_get_capType(handlerCap) == cap_null_cap);
        return false;
    }

    bool_t canDoneate = false;
    sendFaultIPC(tptr, handlerCap, canDoneate);
    return true;
}
#endif /* CONFIG_KERNEL_MCS */

void handleFault(tcb_t *tptr)
{
    seL4_Fault_t fault = current_fault;
    tptr->tcbFault = fault;
    if (seL4_Fault_get_seL4_FaultType(fault) == seL4_Fault_CapFault) {
        tptr->tcbLookupFailure = current_lookup_fault;
    }

    /* get fault hanlder */
    cap_t handlerCap;
#ifdef CONFIG_KERNEL_MCS
    handlerCap = TCB_PTR_CTE_PTR(tptr, tcbFaultHandler)->cap;
#else
    lookupCap_ret_t lu_ret = lookupCap(tptr, tptr->tcbFaultHandler);
    if (lu_ret.status == EXCEPTION_NONE) {
        handlerCap = lu_ret.cap
    } else {
        /* lookup failed */
        handlerCap = cap_null_cap_new();
        // ToDo: Seems we are setting this just for the error printing, as
        //       nobody else cares about this practically?
        //       Also, why don't we set current_lookup_fault here also?
        current_fault = seL4_Fault_CapFault_new(handlerCPtr, false);
    }
#endif

    if (isValidFaultHandlerEp(handlerCap)) {
#ifdef CONFIG_KERNEL_MCS
        bool_t canDonate = (tptr->tcbSchedContext != NULL);
        sendFaultIPC(handlerCap, tptr, canDonate);
#else
        sendIPC(true, /* blocking */
                true, /* do a call */
                cap_endpoint_cap_get_capEPBadge(handlerCap),
                cap_endpoint_cap_get_capCanGrant(handlerCap),
                true, /* can grant reply */
                tptr,
                EP_PTR(cap_endpoint_cap_get_capEPPtr(handlerCap)));
#endif
        return;
    }

    /*
     * There is no fault handler that could be notified about the fault, log
     * some fault details and suspend the faulting thread.
     */

    // ToDo: seeing anything besides a null cap here seems a user error?
    assert(cap_get_capType(handlerCap) == cap_null_cap);
#ifndef CONFIG_KERNEL_MCS
    // ToDo: Seems we are setting this just for the error printing, as nobody
    //       else cares about this practically?
    current_fault = seL4_Fault_CapFault_new(handlerCPtr, false);
    current_lookup_fault = lookup_fault_missing_capability_new(0);
#endif

#ifdef CONFIG_PRINTING
    printFaultHandlerError(tptr, fault);
#endif

    setThreadState(tptr, ThreadState_Inactive);
}
