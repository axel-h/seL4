/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>

#ifdef CONFIG_PRINTING

#include <types.h>
#include <object.h>
#include <api/syscall.h>
#include <api/debug.h>
#include <kernel/thread.h>
#include <benchmark/benchmark_track.h>

/* This function is also available in release builds when printing is enabled,
 * in this case it will not print any register content to avoid leaking
 * potentially sensitive data.
 */
void print_unhandled_fault(tcb_t *tptr, seL4_Fault_t fault, exception_t status)
{
    const char *name = config_ternary(CONFIG_DEBUG_BUILD,
                                      TCB_PTR_DEBUG_PTR(tptr)->tcbName,
                                      null);
    printf("\n"
           "\n"
           "## ==============================================================\n"
           "## FAULT at PC=0x%"SEL4_PRIx_word" in thread %p%s%s%s\n"
           "## Cause: ", /* message follows here */
           getRestartPC(tptr), tptr,
           name ? " (" : "", name, name ? ")" : "");

    word_t fault_type = seL4_Fault_get_seL4_FaultType(fault);
    switch (fault_type) {
    case seL4_Fault_NullFault:
        printf("null fault");
        break;
    case seL4_Fault_CapFault:
        printf("cap fault in %s phase at address 0x%"SEL4_PRIx_word,
               seL4_Fault_CapFault_get_inReceivePhase(fault) ? "receive" : "send",
               (word_t)seL4_Fault_CapFault_get_address(fault));
        break;
    case seL4_Fault_VMFault:
        printf("access fault on %s at address 0x%"SEL4_PRIx_word", "
               "status 0x%"SEL4_PRIx_word,
               seL4_Fault_VMFault_get_instructionFault(fault) ? "code" : "data",
               (word_t)seL4_Fault_VMFault_get_address(fault),
               (word_t)seL4_Fault_VMFault_get_FSR(fault));
        break;
    case seL4_Fault_UnknownSyscall:
        printf("unknown syscall %"SEL4_PRIu_word,
               (word_t)seL4_Fault_UnknownSyscall_get_syscallNumber(fault));
        break;
    case seL4_Fault_UserException:
        printf("user exception %"SEL4_PRIu_word", code 0x%"SEL4_PRIx_word,
               (word_t)seL4_Fault_UserException_get_number(fault),
               (word_t)seL4_Fault_UserException_get_code(fault));
        break;
#ifdef CONFIG_KERNEL_MCS
    case seL4_Fault_Timeout:
        printf("timeout for badge %"PRIu64,
               seL4_Fault_Timeout_get_badge(fault));
        break;
#endif /* CONFIG_KERNEL_MCS */
    default:
        printf("unknown fault 0x%"SEL4_PRIu_word, fault_type);
        break;
    } /* end switch (fault_type) */

    printf("\n## Thread suspended, no userland fault handler"
           " (code %"SEL4_PRIu_word")\n",
           status);
    /* Thread registers are printid in debug builds only. */

#ifdef CONFIG_DEBUG_BUILD

    printf("## State snapshot:\n");

#if CONFIG_WORD_SIZE == 32
#define PRI_reg "%08"SEL4_PRIx_word
#define PRI_reg_param(r)    r
#elif CONFIG_WORD_SIZE == 64
#define PRI_reg "%08x'%08x"
#define PRI_reg_param(r)    (unsigned int)((r) >> 32), (unsigned int)(r)
#else
#error "unsupported CONFIG_WORD_SIZE"
#endif

    user_context_t *user_ctx = &(tptr->tcbArch.tcbContext);
    /* Dynamically adapt spaces to max register name len per column. */
    unsigned int max_reg_name_len[2] = {0};
    for (unsigned int i = 0; i < ARRAY_SIZE(user_ctx->registers); i++) {
        int col = i & 1;
        unsigned int l = strnlen(register_names[i], 20);
        max_reg_name_len[col] = MAX(l, max_reg_name_len[col]);
    }
    const unsigned int num_regs = ARRAY_SIZE(user_ctx->registers);
    for (unsigned int i = 0; i < num_regs; i++) {
        int col = i & 1;
        printf("%s   %*s: 0x"PRI_reg"%s",
               col ? "" : "##",
               max_reg_name_len[col],
               register_names[i],
               PRI_reg_param(user_ctx->registers[i]),
               (col || (i + 1 == num_regs)) ? "\n" : "");
    }
    printf("##\n\nStack trace:\n");
    Arch_userStackTrace(tptr);

#endif /* CONFIG_DEBUG_BUILD */
}

#ifdef CONFIG_DEBUG_BUILD

void debug_printKernelEntryReason(void)
{
    printf("\nKernel entry via ");
    switch (ksKernelEntry.path) {
    case Entry_Interrupt:
        printf("Interrupt, irq %lu\n", (unsigned long) ksKernelEntry.word);
        break;
    case Entry_UnknownSyscall:
        printf("Unknown syscall, word: %lu", (unsigned long) ksKernelEntry.word);
        break;
    case Entry_VMFault:
        printf("VM Fault, fault type: %lu\n", (unsigned long) ksKernelEntry.word);
        break;
    case Entry_UserLevelFault:
        printf("User level fault, number: %lu", (unsigned long) ksKernelEntry.word);
        break;
#ifdef CONFIG_HARDWARE_DEBUG_API
    case Entry_DebugFault:
        printf("Debug fault. Fault Vaddr: 0x%lx", (unsigned long) ksKernelEntry.word);
        break;
#endif
    case Entry_Syscall:
        printf("Syscall, number: %ld, %s\n", (long) ksKernelEntry.syscall_no, syscall_names[ksKernelEntry.syscall_no]);
        if (ksKernelEntry.syscall_no == -SysSend ||
            ksKernelEntry.syscall_no == -SysNBSend ||
            ksKernelEntry.syscall_no == -SysCall) {

            printf("Cap type: %lu, Invocation tag: %lu\n", (unsigned long) ksKernelEntry.cap_type,
                   (unsigned long) ksKernelEntry.invocation_tag);
        }
        break;
#ifdef CONFIG_ARCH_ARM
    case Entry_VCPUFault:
        printf("VCPUFault\n");
        break;
#endif
#ifdef CONFIG_ARCH_x86
    case Entry_VMExit:
        printf("VMExit\n");
        break;
#endif
    default:
        printf("Unknown\n");
        break;

    }
}

void debug_printUserState(void)
{
    tcb_t *tptr = NODE_STATE(ksCurThread);
    printf("Current thread: %s\n", TCB_PTR_DEBUG_PTR(tptr)->tcbName);
    printf("Next instruction adress: %lx\n", getRestartPC(tptr));
    printf("Stack:\n");
    Arch_userStackTrace(tptr);
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
        for (tcb_t *tcb = NODE_STATE(ksDebugTCBs);
             tcb != NULL;
             tcb = TCB_PTR_DEBUG_PTR(tcb)->tcbDebugNext) {
            num_tcb++;
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

    for (tcb_t *tcb = specific_tcb ? specific_tcb : NODE_STATE(ksDebugTCBs);
         tcb != NULL;
         tcb = specific_tcb ? NULL : TCB_PTR_DEBUG_PTR(tcb)->tcbDebugNext) {
        printf("  %-*"SEL4_PRIx_word " "
               "| %-13s "
               "| %-*"SEL4_PRIx_word" "
               "| %4" SEL4_PRIu_word " "
               config_ternary(CONFIG_ENABLE_SMP_SUPPORT, "| %4" SEL4_PRIu_word " ", "")
               config_ternary(CONFIG_KERNEL_MCS, "| %4s ", "")
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
