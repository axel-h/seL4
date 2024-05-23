/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <util.h>
#include <linker.h>
#include <api/types.h>
#include <api/syscall.h>
#include <api/types.h>
#include <smp/lock.h>
#include <arch/machine/hardware.h>
#include <machine/fpu.h>

void slowpath(syscall_t syscall)
NORETURN;

static inline
void fastpath_call(word_t cptr, word_t r_msgInfo)
NORETURN;

static inline
#ifdef CONFIG_KERNEL_MCS
void fastpath_reply_recv(word_t cptr, word_t r_msgInfo, word_t reply)
#else
void fastpath_reply_recv(word_t cptr, word_t r_msgInfo)
#endif
NORETURN;

/* Use macros to not break verification */
#define endpoint_ptr_get_epQueue_tail_fp(ep_ptr) TCB_PTR(endpoint_ptr_get_epQueue_tail(ep_ptr))
#define cap_vtable_cap_get_vspace_root_fp(vtable_cap) PTE_PTR(cap_page_table_cap_get_capPTBasePtr(vtable_cap))

static inline void FORCE_INLINE switchToThread_fp(tcb_t *thread, pte_t *vroot, pte_t stored_hw_asid)
{
    asid_t asid = (asid_t)(stored_hw_asid.words[0]);

    setVSpaceRoot(addrFromPPtr(vroot), asid);

    NODE_STATE(ksCurThread) = thread;
}

static inline void mdb_node_ptr_mset_mdbNext_mdbRevocable_mdbFirstBadged(
    mdb_node_t *node_ptr, word_t mdbNext,
    word_t mdbRevocable, word_t mdbFirstBadged)
{
    node_ptr->words[1] = mdbNext | (mdbRevocable << 1) | mdbFirstBadged;
}

static inline void mdb_node_ptr_set_mdbPrev_np(mdb_node_t *node_ptr, word_t mdbPrev)
{
    node_ptr->words[0] = mdbPrev;
}

static inline bool_t isValidVTableRoot_fp(cap_t vspace_root_cap)
{
    return cap_capType_equals(vspace_root_cap, cap_page_table_cap) &&
           cap_page_table_cap_get_capPTIsMapped(vspace_root_cap);
}

/* This is an accelerated check that msgLength, which appears
   in the bottom of the msgInfo word, is <= 4 and that msgExtraCaps
   which appears above it is zero. We are assuming that n_msgRegisters == 4
   for this check to be useful. By masking out the bottom 3 bits, we are
   really checking that n + 3 <= MASK(3), i.e. n + 3 <= 7 or n <= 4. */
compile_assert(n_msgRegisters_eq_4, n_msgRegisters == 4)
static inline int
fastpath_mi_check(word_t msgInfo)
{
    return (msgInfo & MASK(seL4_MsgLengthBits + seL4_MsgExtraCapBits)) > 4;
}

static inline void fastpath_copy_mrs(word_t length, tcb_t *src, tcb_t *dest)
{
    word_t i;
    register_t reg;

    /* assuming that length < n_msgRegisters */
    for (i = 0; i < length; i ++) {
        /* assuming that the message registers simply increment */
        reg = msgRegisters[0] + i;
        setRegister(dest, reg, getRegister(src, reg));
    }
}

static inline int fastpath_reply_cap_check(cap_t cap)
{
    return cap_capType_equals(cap, cap_reply_cap);
}

/** DONT_TRANSLATE */
static inline void NORETURN FORCE_INLINE fastpath_restore(word_t badge, word_t msgInfo, tcb_t *cur_thread)
{
    c_exit_hook();
    NODE_UNLOCK_IF_HELD;

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

    /* The RISC-V A-Extension defines the LR/SC instruction pair for conditional
     * stores using a reservation. Explicitly clearing any reservations is
     * considered not necessary on the fastpath for functional correctness,
     * since user threads are not supposed to perform IPC/Signal operations in
     * the middle of LR/SC sequences. Clearing should not be needed there for
     * dataflow reasons either, as the reservation can be considered a message
     * register (although impossible to use gainfully on most implementations).
     */

    register word_t reg_a0 asm("a0") = badge;
    register word_t reg_a1 asm("a1") = msgInfo;
    register word_t reg_t6 asm("t6") = (word_t)regs;

    asm volatile(
        LOAD_REG("ra",  0,  "t6") /* x1  */
        LOAD_REG("sp",  1,  "t6") /* x2  */
        LOAD_REG("gp",  2,  "t6") /* x3  */
        LOAD_REG("tp",  3,  "t6") /* x4  */
        LOAD_REG("t0",  4,  "t6") /* x5  */
        LOAD_REG("t1",  5,  "t6") /* x6  */
        LOAD_REG("t2",  6,  "t6") /* x7  */
        LOAD_REG("s0",  7,  "t6") /* x8  */
        LOAD_REG("s1",  8,  "t6") /* x9  */
        /* a0/x10, a1/x11 are already set */
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
        : "r"(reg_a0), "r"(reg_a1), "r"(reg_t6)
        : "memory"
    );

    UNREACHABLE();
}
