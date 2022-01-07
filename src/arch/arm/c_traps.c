/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <arch/kernel/traps.h>
#include <arch/object/vcpu.h>
#include <arch/machine/registerset.h>
#include <api/syscall.h>
#include <machine/fpu.h>
#include <arch/machine.h>
#include <benchmark/benchmark.h>

void VISIBLE NORETURN c_handle_undefined_instruction(void)
{
    NODE_LOCK_SYS;
    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry(Entry_UserLevelFault,
                       getRegister(NODE_STATE(ksCurThread), NextIP));
#endif

#if defined(CONFIG_HAVE_FPU) && defined(CONFIG_ARCH_AARCH32)
    /* We assume the first fault is a FP exception and enable FPU, if not already enabled */
    if (!isFpuEnable()) {
        handleFPUFault();

        /* Restart the FP instruction that cause the fault */
        setNextPC(NODE_STATE(ksCurThread), getRestartPC(NODE_STATE(ksCurThread)));
    } else {
        handleUserLevelFault(0, 0);
    }

    restore_user_context();
    UNREACHABLE();
#endif

    /* There's only one user-level fault on ARM, and the code is (0,0) */
#ifdef CONFIG_ARCH_AARCH32
    handleUserLevelFault(0, 0);
#else
    handleUserLevelFault(getESR(), 0);
#endif
    restore_user_context();
    UNREACHABLE();
}

#if defined(CONFIG_HAVE_FPU) && defined(CONFIG_ARCH_AARCH64)
void VISIBLE NORETURN c_handle_enfp(void)
{
    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry(Entry_UserLevelFault,
                       getRegister(NODE_STATE(ksCurThread), NextIP));
#endif

    handleFPUFault();
    restore_user_context();
    UNREACHABLE();
}
#endif /* CONFIG_HAVE_FPU */

#ifdef CONFIG_EXCEPTION_FASTPATH
void NORETURN vm_fault_slowpath(vm_fault_type_t type)
{
    handleVMFaultEvent(type);
    restore_user_context();
    UNREACHABLE();
}
#endif

static inline void NORETURN c_handle_vm_fault(vm_fault_type_t type)
{
    NODE_LOCK_SYS;
    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry(Entry_VMFault,
                       getRegister(NODE_STATE(ksCurThread), NextIP));
    // ksKernelEntry.is_fastpath = false;
#endif

#ifdef CONFIG_EXCEPTION_FASTPATH
    fastpath_vm_fault(type);
# else
    handleVMFaultEvent(type);
    restore_user_context();
#endif
    UNREACHABLE();
}

void VISIBLE NORETURN c_handle_data_fault(void)
{
    c_handle_vm_fault(seL4_DataFault);
}

void VISIBLE NORETURN c_handle_instruction_fault(void)
{
    c_handle_vm_fault(seL4_InstructionFault);
}

void VISIBLE NORETURN c_handle_interrupt(void)
{
    NODE_LOCK_IRQ_IF(IRQT_TO_IRQ(getActiveIRQ()) != irq_remote_call_ipi);
    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry(Entry_Interrupt, IRQT_TO_IRQ(getActiveIRQ()));
#endif

    handleInterruptEntry();
    restore_user_context();
}

void NORETURN slowpath(syscall_t syscall)
{
    if (unlikely(syscall < SYSCALL_MIN || syscall > SYSCALL_MAX)) {
#ifdef TRACK_KERNEL_ENTRIES
        ksKernelEntry.path = Entry_UnknownSyscall;
        /* ksKernelEntry.word word is already set to syscall */
#endif /* TRACK_KERNEL_ENTRIES */
        /* Contrary to the name, this handles all non-standard syscalls used in
         * debug builds also.
         */
        handleUnknownSyscall(syscall);
    } else {
#ifdef TRACK_KERNEL_ENTRIES
        ksKernelEntry.is_fastpath = 0;
#endif /* TRACK KERNEL ENTRIES */
        handleSyscall(syscall);
    }

    restore_user_context();
    UNREACHABLE();
}

void VISIBLE c_handle_syscall(word_t cptr, word_t msgInfo, syscall_t syscall)
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry_syscall(syscall, cptr, msgInfo, 0);
#endif

    slowpath(syscall);
    UNREACHABLE();
}

#ifdef CONFIG_FASTPATH
ALIGN(L1_CACHE_LINE_SIZE)
void VISIBLE c_handle_fastpath_call(word_t cptr, word_t msgInfo)
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry_syscall(SysCall, cptr, msgInfo, 1);
#endif

    fastpath_call(cptr, msgInfo);
    UNREACHABLE();
}

#ifdef CONFIG_KERNEL_MCS
#ifdef CONFIG_SIGNAL_FASTPATH
ALIGN(L1_CACHE_LINE_SIZE)
void VISIBLE c_handle_fastpath_signal(word_t cptr, word_t msgInfo)
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef TRACK_KERNEL_ENTRIES
    benchmark_debug_syscall_start(cptr, msgInfo, SysCall);
    ksKernelEntry.is_fastpath = 1;
#endif /* DEBUG */
    fastpath_signal(cptr, msgInfo);
    UNREACHABLE();
}
#endif /* CONFIG_SIGNAL_FASTPATH */
#endif /* CONFIG_KERNEL_MCS */

ALIGN(L1_CACHE_LINE_SIZE)
#ifdef CONFIG_KERNEL_MCS
void VISIBLE c_handle_fastpath_reply_recv(word_t cptr, word_t msgInfo, word_t reply)
#else
void VISIBLE c_handle_fastpath_reply_recv(word_t cptr, word_t msgInfo)
#endif
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry_syscall(SysReplyRecv, cptr, msgInfo, 1);
#endif

#ifdef CONFIG_KERNEL_MCS
    fastpath_reply_recv(cptr, msgInfo, reply);
#else
    fastpath_reply_recv(cptr, msgInfo);
#endif
    UNREACHABLE();
}

#endif

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
VISIBLE NORETURN void c_handle_vcpu_fault(word_t hsr)
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef ENABLE_TRACE_KERNEL_ENTRY_EXIT
    trace_kernel_entry(Entry_VCPUFault, hsr);
#endif

    handleVCPUFault(hsr);
    restore_user_context();
    UNREACHABLE();
}
#endif /* CONFIG_ARM_HYPERVISOR_SUPPORT */
