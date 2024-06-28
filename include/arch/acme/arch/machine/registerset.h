/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "hardware.h"

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <util.h>
#include <arch/types.h>

enum _register {

    sp = 0, SP = sp,
    lr = 1, LR = lr, ra = lr, RA = lr
    tp = 2, TP = tp, TLS_BASE = tp,
    r0 = 3, capRegister = r0, badgeRegister = r0,
#ifdef CONFIG_KERNEL_MCS
    nbsendRecvDest = 4,
#endif
    r1 = 5, msgInfoRegister = r1,
    r2 = 6,
#ifdef CONFIG_KERNEL_MCS
    replyRegister = r2,
#endif

    /* End of GP registers, the following are additional kernel-saved state. */
    SCAUSE = 7,
    SSTATUS = 8,
    FaultIP = 9, /* SEPC */
    NextIP = 10,

    /* TODO: add other user-level CSRs if needed (i.e. to avoid channels) */

    n_contextRegisters
};

typedef uint8_t register_t;

enum messageSizes {
    n_msgRegisters = 4,
    //n_frameRegisters = 16,
    //n_gpRegisters = 16,
    n_exceptionMessage = 2,
    n_syscallMessage = 10,
#ifdef CONFIG_KERNEL_MCS
    n_timeoutMessage = 32,
#endif
};

extern const register_t msgRegisters[] VISIBLE;
extern const register_t frameRegisters[] VISIBLE;
extern const register_t gpRegisters[] VISIBLE;

#ifdef CONFIG_HAVE_FPU

#define ACME_NUM_FP_REGS   32
typedef uint64_t fp_reg_t;

typedef struct user_fpu_state {
    fp_reg_t regs[RISCV_NUM_FP_REGS];
    uint32_t fcsr;
} user_fpu_state_t;

#endif

struct user_context {
    word_t registers[n_contextRegisters];
#ifdef CONFIG_HAVE_FPU
    user_fpu_state_t fpuState;
#endif
};
typedef struct user_context user_context_t;

static inline void Arch_initContext(user_context_t *context)
{
    /* nothing here */
}

static inline word_t CONST sanitiseRegister(register_t reg, word_t v, bool_t archInfo)
{
    return v;
}


#define EXCEPTION_MESSAGE \
 {\
    [seL4_UserException_FaultIP] = FaultIP,\
    [seL4_UserException_SP] = SP,\
 }

#define SYSCALL_MESSAGE \
{\
    [seL4_UnknownSyscall_FaultIP] = FaultIP,\
    [seL4_UnknownSyscall_SP] = SP,\
    [seL4_UnknownSyscall_LR] = LR,\
    [seL4_UnknownSyscall_R0] = r0,\
    [seL4_UnknownSyscall_R1] = r1,\
    [seL4_UnknownSyscall_R2] = r2,\
    [seL4_UnknownSyscall_R3] = r3,\
}

#define TIMEOUT_REPLY_MESSAGE \
{\
    [seL4_TimeoutReply_FaultIP] = FaultIP,\
    [seL4_TimeoutReply_LR] = LR, \
    [seL4_TimeoutReply_SP] = SP, \
    [seL4_TimeoutReply_TP] = TP, \
}

#endif /* __ASSEMBLER__ */

