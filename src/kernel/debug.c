/*
 * Copyright 2014, General Dynamics C4 Systems
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>

#ifdef CONFIG_PRINTING

#include <types.h>
#include <util.h>
#include <api/debug.h>
#include <machine/io.h>
#include <kernel/thread.h>


/* This function is also available in release builds when printing is enabled,
 * in this case it will not print any register content to avoid leaking
 * potentially sensitive data.
 */
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
static void debug_printThreadStack(tcb_t *tptr, const char *prefix)
{
    /* We don't make any assumption about how exactly a thread stack works on a
     * specific architecture. There is also no need to optimize the speed of the
     * dump, as printing to a serial port would be the bottle neck eventually.
     *
     * Assuming a stack is a block of memory in the vspace and the stack pointer
     * is growing to lower addresses, this would also work:
     *
     *     stack_base = Arch_readWordFromThreadStack(tptr, 0)
     *     loop:
     *         ret = Arch_readWordFromVSpace(
     *                   stack_base.vspace_root,
     *                   stack_base.vaddr + (i * sizeof(word_t)
     *
     * But we could also just have an arch specific helper that popultes
     *
     *     word_t stack_trace[CONFIG_USER_STACK_TRACE_LENGTH];
     *   Arch_getStackTrce(tptr &stack_trace, CONFIG_USER_STACK_TRACE_LENGTH);
     *
     */

    for (int i = 0; i < CONFIG_USER_STACK_TRACE_LENGTH; i++) {
        readWordFromStack_ret_t ret = Arch_readWordFromThreadStack(tptr, i);
        printf("%s0x%"SEL4_PRIx_word": ", prefix, ret.vaddr);
        switch(ret.status) {
        case VSPACE_INVALID_ROOT:
            /* If the thread's vspace can't be resolved there is no point to
             * continue trying to dump the stack. We have printed the value
             * stack pointer already, so
             */
            printf("invalid vspace\n");
            return;
        case VSPACE_INVALID_ALIGNMENT:
            printf("invalid alignment (phys addr 0x%"SEL4_PRIx_word")", ret.paddr);
            break;
        case VSPACE_LOOKUP_FAILED:
            printf("inaccessible (phys addr 0x%"SEL4_PRIx_word")", ret.paddr);
            break;
        case VSPACE_ACCESS_SUCCESSFUL:
            printf("0x%-*"SEL4_PRIx_word, CONFIG_WORD_SIZE / 4, ret.value);
            // #if 32 bit
            //   printf("[%2u] 0x%"SEL4_PRIx_word": 0x%08"SEL4_PRIx_word"\n",
            //       i, address, *pValue);
            // #if 64 bit
            //   /* Print 64 bit values from the stack as 0x01234567'89abcdef,
            //    * this is much easier to read. */
            //   printf("[%2u] 0x%"SEL4_PRIx_word": 0x%08x'%08x\n",
            //       i, address, (uint32_t)(*pValue >> 32), (uint32_t)(*pValue));
            // #else
            // #error "128 bit machine?"
            // #endif
            break;
        } /* end switch(ret.status) */
        printf("\n");
    }
}

/* This function is also available in release builds when printing is enabled,
 * in this case it will not print any register content to avoid leaking
 * potentially sensitive data.
 */
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
        printf("%s%*s: 0x"PRI_reg"%s",
               col ? "" : "##  ", max_reg_name_len[col] + 2, register_names[i],
               PRI_reg_param(reg), (col || is_last) ? "\n" : "");
    }
    printf("## Stack trace:\n");
    debug_printThreadStack(tptr, "##    ");

#endif /* CONFIG_DEBUG_BUILD */

    printf("\nThread suspended, no userland fault handler\n");
}

#endif /* CONFIG_PRINTING */


#ifdef CONFIG_DEBUG_BUILD

void debug_printKernelEntryReason(void)
{
    printf("\nKernel entry via ");

    switch (ksKernelEntry.path) {
    case Entry_Interrupt:
        printf("interrupt %"SEL4_PRIu_word"\n", (seL4_Word)ksKernelEntry.word);
        break;
    case Entry_UnknownSyscall:
        printf("unknown syscall %"SEL4_PRIu_word"\n",
               (seL4_Word)ksKernelEntry.word);
        break;
    case Entry_VMFault:
        printf("VM fault %"SEL4_PRIu_word"\n", (seL4_Word)ksKernelEntry.word);
        break;
    case Entry_UserLevelFault:
        printf("user level fault (number: %"SEL4_PRIu_word")\n",
               (seL4_Word)ksKernelEntry.word);
        break;
#ifdef CONFIG_HARDWARE_DEBUG_API
    case Entry_DebugFault:
        printf("debug fault (vaddr: 0x%"SEL4_PRIu_word")\n",
               (seL4_Word)ksKernelEntry.word);
        break;
#endif
    case Entry_Syscall:
        printf("syscall %"SEL4_PRIu_word" (%s)",
               (seL4_Word)ksKernelEntry.syscall_no,
               syscall_names[ksKernelEntry.syscall_no]);
        switch (-ksKernelEntry.syscall_no) {
        case SysSend:
        case SysNBSend:
        case SysCall:
            printf(", cap type %"SEL4_PRIu_word
                   ", invocation tag %"SEL4_PRIu_word"\n",
                   (seL4_Word)ksKernelEntry.cap_type,
                   (seL4_Word)ksKernelEntry.invocation_tag);
            break;
        default:
            break; /* nothing special to log for other syscalls */
        } /* end switch (-ksKernelEntry.syscall_no) */
        break;
#ifdef CONFIG_ARCH_ARM
    case Entry_VCPUFault:
        printf("VCPU Fault\n");
        break;
#endif /* CONFIG_ARCH_ARM */
#ifdef CONFIG_ARCH_x86
    case Entry_VMExit:
        printf("VM Exit\n");
        break;
#endif /* CONFIG_ARCH_x86*/
    default:
        printf("unknown path %"SEL4_PRIu_word"\n",
               (seL4_Word)ksKernelEntry.path);
        break;
    } /* end switch (ksKernelEntry.path) */
}

void debug_printUserState(void)
{
    tcb_t *tptr = NODE_STATE(ksCurThread);
    /* This function only exists for CONFIG_DEBUG_BUILD, thus tcbName exists */
    printf("Current thread: %s\n", TCB_PTR_DEBUG_PTR(tptr)->tcbName);
    printf("Next instruction address: 0x%"SEL4_PRIx_word"\n", getRestartPC(tptr));
    printf("Stack:\n");
    debug_printThreadStack(tptr, "  ");
}

static const char *string_from_ThreadState(thread_state_t state)
{
    switch (thread_state_get_tsType(state)) {
    case ThreadState_Inactive:
        return "inactive";
    case ThreadState_Running:
        return "running";
    case ThreadState_Restart:
        return "restart";
    case ThreadState_BlockedOnReceive:
        return "blocked/recv";
    case ThreadState_BlockedOnSend:
        return "blocked/send";
    case ThreadState_BlockedOnReply:
        return "blocked/reply";
    case ThreadState_BlockedOnNotification:
        return "blocked/ntfn";
#ifdef CONFIG_VTX
    case ThreadState_RunningVM:
        return "running VM";
#endif
    case ThreadState_IdleThreadState:
        return "idle";
    default:
        break;
    } /* end switch */

    return "???";
}

static void debug_dumpSchedulerEx(tcb_t *specific_tcb)
{
    /* enough dashes to fill space used by an u64 in hex */
    static const char *DASHES = "----------------";
    unsigned int num_tcb = 0;

    if (!specific_tcb) {
        /* ToDo: if we iterate over all cores, we should consider getting the
         *       BKL to ensure the other cores are not changing the state in
         *       parallel, so strange things get printed or this even crashes.
         */
        for (int i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
            for (tcb_t *tcb = NODE_STATE_ON_CORE(ksDebugTCBs, i);
                 tcb != NULL;
                 tcb = TCB_PTR_DEBUG_PTR(tcb)->tcbDebugNext) {
                num_tcb++;
            }
        }
        printf("Dump of all TCBs (%u):\n", num_tcb);
    }

    printf(
        "  %-*s "
        "| %-13s "
        "| %-*s "
        "| Prio "
        config_ternary(CONFIG_ENABLE_SMP_SUPPORT, "| Core ", "")
        config_ternary(CONFIG_KERNEL_MCS, "| RelQ ", "")
        "| Name"
        "\n"
        "  %.*s-" /* dashes for length of SEL4_PRIx_word */
        "+---------------"
        "+-%.*s-" /* dashes for length of SEL4_PRIx_word */
        "+------"
        config_ternary(CONFIG_ENABLE_SMP_SUPPORT, "+------", "")
        config_ternary(CONFIG_KERNEL_MCS, "+------", "")
        "+----------------"
        "\n",
        CONFIG_WORD_SIZE / 4, "TCB",
        "State",
        CONFIG_WORD_SIZE / 4, "PC",
        CONFIG_WORD_SIZE / 4, DASHES,
        CONFIG_WORD_SIZE / 4, DASHES);

    for (int i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
        for (tcb_t *tcb = specific_tcb ? specific_tcb :  NODE_STATE_ON_CORE(ksDebugTCBs, i);
             tcb != NULL;
             tcb = specific_tcb ? NULL : TCB_PTR_DEBUG_PTR(tcb)->tcbDebugNext) {
            printf("  %-*"SEL4_PRIx_word " "
                   "| %-13s "
                   "| %-*"SEL4_PRIx_word" "
                   "| %4" SEL4_PRIu_word " "
#ifdef CONFIG_ENABLE_SMP_SUPPORT
                   "| %4" SEL4_PRIu_word " "
#endif
#ifdef CONFIG_KERNEL_MCS
                   "| %4s "
#endif
                   "| %s"
                   "\n",
                   CONFIG_WORD_SIZE / 4, (word_t)tcb,
                   string_from_ThreadState(tcb->tcbState),
                   CONFIG_WORD_SIZE / 4, getRestartPC(tcb),
                   tcb->tcbPriority,
#ifdef CONFIG_ENABLE_SMP_SUPPORT
                   tcb->tcbAffinity,
#endif
#ifdef CONFIG_KERNEL_MCS
                   thread_state_get_tcbInReleaseQueue(tcb->tcbState) ? "yes" : "no",
#endif
                   TCB_PTR_DEBUG_PTR(tcb)->tcbName);
        }
    }
}

void debug_printTCB(tcb_t *tcb)
{
    assert(tcb);
    debug_dumpSchedulerEx(tcb);
}

void debug_dumpScheduler(void)
{
    debug_dumpSchedulerEx(NULL);
}

#endif /* CONFIG_DEBUG_BUILD */
#endif /* CONFIG_PRINTING */
