/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <model/statedata.h>
#include <arch/fastpath/fastpath.h>
#include <arch/kernel/traps.h>
#include <machine/debug.h>
#include <api/syscall.h>
#include <util.h>
#include <arch/machine/hardware.h>
#include <machine/fpu.h>

#include <benchmark/benchmark_track.h>
#include <benchmark/benchmark_utilisation.h>

/** DONT_TRANSLATE */
void VISIBLE NORETURN restore_user_context(void)
{
    c_exit_hook();
    NODE_UNLOCK_IF_HELD;

    tcb_t *cur_thread = NODE_STATE(ksCurThread);
    word_t *regs = cur_thread->tcbArch.tcbContext.registers;

    write_sstatus(regs[SSTATUS]);
    write_sepc(regs[NextIP]);

#ifdef ENABLE_SMP_SUPPORT
    /* CRS sscratch permanently holds the current core's stack pointer for
     * kernel entry. Put the current thread's reg space as first element there,
     * so it can be obtained easiy on the next entry.
     */
    word_t *kernel_stack = (word_t *)read_sscratch();
    kernel_stack[-1] = (word_t)regs;
#else
    /* CRS sscratch holds the pointer to the regs of the current thread, so it
     * can be obtained easiy on the next entry.
     */
    write_sscratch((word_t)regs);
#endif

#ifdef CONFIG_HAVE_FPU
    lazyFPURestore(cur_thread);
    set_tcb_fs_state(cur_thread, isFpuEnable());
#endif

    register word_t reg_t6 asm("t6") = (word_t)regs;

    asm volatile(
        LOAD_REG("ra",  0,  "t6") /* x1  */

        /* The RISC-V A-Extension defines the LR/SC instruction pair for
         * conditional stores using a reservation. We have to clear any existing
         * reservation here, because we don't know where the user thread got
         * interrupted. Clearing does not happen automatically, the following
         * core-specific implementation behavior is known:
         * - SAIL Model: The current (May/2024) model clears the reservation
         *      on traps, xRET and WFI. There is a discussion to remove this and
         *      align it with the common RISC-V silicon implementations, where
         *      only an explict sc.w instruction clears the reservation.
         * - SiFive U54/U74, Codasip A700, Ariane:
         *      Only an explicit sc.w instruction is guaranteed to clear any
         *      reservations.
         * - XuanTie C906/C920: ??
         * - RocketChip: Reservations time out after at most 80 cycles. Since
         *      there is no path through the scheduler that takes less time than
         *      that, no manual maintenance is be necessary, because no issues
         *      are observable.
         *
         * Explictly clear any outstanding reservations with a dummy sc.w on
         * ra/t6. The code above has put cur_thread[0] into ra, so simply
         * write it back. This store operation is very likely to fail, because
         * there is no reservation. But if it succeeds, it does no harm.
         */
        "sc.w zero, ra, (t6)\n"

        LOAD_REG("sp",  1,  "t6") /* x2  */
        LOAD_REG("gp",  2,  "t6") /* x3  */
        LOAD_REG("tp",  3,  "t6") /* x4  */
        LOAD_REG("t0",  4,  "t6") /* x5  */
        LOAD_REG("t1",  5,  "t6") /* x6  */
        LOAD_REG("t2",  6,  "t6") /* x7  */
        LOAD_REG("s0",  7,  "t6") /* x8  */
        LOAD_REG("s1",  8,  "t6") /* x9  */
        LOAD_REG("a0",  9,  "t6") /* x10 */
        LOAD_REG("a1",  10, "t6") /* x11 */
        LOAD_REG("a2",  11, "t6") /* x12 */
        LOAD_REG("a3",  12, "t6") /* x13 */
        LOAD_REG("a4",  13, "t6") /* x14 */
        LOAD_REG("a5",  14, "t6") /* x15 */
        LOAD_REG("a6",  15, "t6") /* x16 */
        LOAD_REG("a7",  16, "t6") /* x17 */
        LOAD_REG("s2",  17, "t6") /* x18 */
        LOAD_REG("s3",  18, "t6") /* x19 */
        LOAD_REG("s4",  19, "t6") /* x20 */
        LOAD_REG("s5",  20, "t6") /* x21 */
        LOAD_REG("s6",  21, "t6") /* x22 */
        LOAD_REG("s7",  22, "t6") /* x23 */
        LOAD_REG("s8",  23, "t6") /* x24 */
        LOAD_REG("s9",  24, "t6") /* x25 */
        LOAD_REG("s10", 25, "t6") /* x26 */
        LOAD_REG("s11", 26, "t6") /* x27 */
        LOAD_REG("t3",  27, "t6") /* x28 */
        LOAD_REG("t4",  28, "t6") /* x29 */
        LOAD_REG("t5",  29, "t6") /* x30 */
        LOAD_REG("t6",  30, "t6") /* x31 */
        "sret"
        : /* no output */
        : "r"(reg_t6)
        : "memory"
    );

    UNREACHABLE();
}

void VISIBLE NORETURN c_handle_interrupt(void)
{
    NODE_LOCK_IRQ_IF(getActiveIRQ() != irq_remote_call_ipi);

    c_entry_hook();

    handleInterruptEntry();

    restore_user_context();
    UNREACHABLE();
}

void VISIBLE NORETURN c_handle_exception(void)
{
    NODE_LOCK_SYS;

    c_entry_hook();

    word_t scause = read_scause();
    switch (scause) {
    case RISCVInstructionAccessFault:
    case RISCVLoadAccessFault:
    case RISCVStoreAccessFault:
    case RISCVLoadPageFault:
    case RISCVStorePageFault:
    case RISCVInstructionPageFault:
        handleVMFaultEvent(scause);
        break;
    default:
#ifdef CONFIG_HAVE_FPU
        if (!isFpuEnable()) {
            /* we assume the illegal instruction is caused by FPU first */
            handleFPUFault();
            setNextPC(NODE_STATE(ksCurThread), getRestartPC(NODE_STATE(ksCurThread)));
            break;
        }
#endif
        handleUserLevelFault(scause, 0);
        break;
    }

    restore_user_context();
    UNREACHABLE();
}

void VISIBLE NORETURN slowpath(syscall_t syscall)
{
    if (unlikely(syscall < SYSCALL_MIN || syscall > SYSCALL_MAX)) {
#ifdef TRACK_KERNEL_ENTRIES
        ksKernelEntry.path = Entry_UnknownSyscall;
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

#ifdef CONFIG_FASTPATH
ALIGN(L1_CACHE_LINE_SIZE)
#ifdef CONFIG_KERNEL_MCS
void VISIBLE c_handle_fastpath_reply_recv(word_t cptr, word_t msgInfo, word_t reply)
#else
void VISIBLE c_handle_fastpath_reply_recv(word_t cptr, word_t msgInfo)
#endif
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef TRACK_KERNEL_ENTRIES
    benchmark_debug_syscall_start(cptr, msgInfo, SysReplyRecv);
    ksKernelEntry.is_fastpath = 1;
#endif /* DEBUG */
#ifdef CONFIG_KERNEL_MCS
    fastpath_reply_recv(cptr, msgInfo, reply);
#else
    fastpath_reply_recv(cptr, msgInfo);
#endif
    UNREACHABLE();
}

ALIGN(L1_CACHE_LINE_SIZE)
void VISIBLE c_handle_fastpath_call(word_t cptr, word_t msgInfo)
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef TRACK_KERNEL_ENTRIES
    benchmark_debug_syscall_start(cptr, msgInfo, SysCall);
    ksKernelEntry.is_fastpath = 1;
#endif /* DEBUG */

    fastpath_call(cptr, msgInfo);

    UNREACHABLE();
}
#endif

void VISIBLE NORETURN c_handle_syscall(word_t cptr, word_t msgInfo, syscall_t syscall)
{
    NODE_LOCK_SYS;

    c_entry_hook();
#ifdef TRACK_KERNEL_ENTRIES
    benchmark_debug_syscall_start(cptr, msgInfo, syscall);
    ksKernelEntry.is_fastpath = 0;
#endif /* DEBUG */
    slowpath(syscall);

    UNREACHABLE();
}
