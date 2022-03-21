/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4/macros.h>

#define seL4_WordBits           64
/* log 2 bits in a word */
#define seL4_WordSizeBits       3

#define seL4_SlotBits           5
#ifdef CONFIG_KERNEL_MCS
#define seL4_NotificationBits   6
#define seL4_ReplyBits          5
#else /* not CONFIG_KERNEL_MCS */
#define seL4_NotificationBits   5
#endif /* [not] CONFIG_KERNEL_MCS */
#define seL4_EndpointBits       4
#define seL4_IPCBufferSizeBits  10
#ifdef CONFIG_HAVE_FPU
#define seL4_TCBBits            11
#else /* not CONFIG_HAVE_FPU */
#define seL4_TCBBits            10
#endif /* [not] CONFIG_HAVE_FPU */


/* In the ACME architecture a page is 4 KiB and each page table uses exactly one
 * page. A VSpace is simply the root page table. Since each page table entry has
 * 2^3 = 8 byte = 64 bit per entry, a page table has 512 entries. Thus each page
 * level can cover overs 2^9 (=512) times the size of one entry and provides 9
 * bits for the address translation.
 */
#define seL4_PageBits               12
#define seL4_PageTableEntryBits      3
#define seL4_PageTableIndexBits      9
#define seL4_PageTableBits          seL4_PageBits
#define seL4_VSpaceBits             seL4_PageTableBits
#define seL4_PT_L1                  (seL4_PageBits + seL4_PageTableIndexBits) /* 21 */
#define seL4_PT_L2                  (seL4_PT_L1 + seL4_PageTableIndexBits) /* 30 */
#define seL4_PT_L3                  (seL4_PT_L2 + seL4_PageTableIndexBits) /* 39 */
#define seL4_PT_L4                  (seL4_PT_L3 + seL4_PageTableIndexBits) /* 48 */
#define seL4_PT_L5                  (seL4_PT_L4 + seL4_PageTableIndexBits) /* 57 */

#define seL4_NumASIDPoolsBits   7
#define seL4_ASIDPoolIndexBits  9
#define seL4_ASIDPoolBits       12

/* Untyped size limits */
#define seL4_MinUntypedBits     4
#define seL4_MaxUntypedBits     38

#ifndef __ASSEMBLER__

typedef enum {
    seL4_VMFault_IP,
    seL4_VMFault_Addr,
    seL4_VMFault_PrefetchFault,
    seL4_VMFault_FSR,
    seL4_VMFault_Length,
} seL4_VMFault_Msg;

typedef enum {
    seL4_UnknownSyscall_FaultIP,
    seL4_UnknownSyscall_SP,
    seL4_UnknownSyscall_LR,
    seL4_UnknownSyscall_R0,
    seL4_UnknownSyscall_R1,
    seL4_UnknownSyscall_R2,
    seL4_UnknownSyscall_R3,
    seL4_UnknownSyscall_Syscall,
    seL4_UnknownSyscall_Length,
} seL4_UnknownSyscall_Msg;

typedef enum {
    seL4_UserException_FaultIP,
    seL4_UserException_SP,
    seL4_UserException_Number,
    seL4_UserException_Code,
    seL4_UserException_Length,
} seL4_UserException_Msg;

#ifdef CONFIG_KERNEL_MCS
typedef enum {
    seL4_TimeoutReply_FaultIP,
    seL4_TimeoutReply_LR,
    seL4_TimeoutReply_SP,
    //seL4_TimeoutReply_GP,
    //seL4_TimeoutReply_s0,
    //seL4_TimeoutReply_s1,
    //seL4_TimeoutReply_s2,
    //seL4_TimeoutReply_s3,
    //seL4_TimeoutReply_s4,
    //seL4_TimeoutReply_s5,
    //seL4_TimeoutReply_s6,
    //seL4_TimeoutReply_s7,
    //seL4_TimeoutReply_s8,
    //seL4_TimeoutReply_s9,
    //seL4_TimeoutReply_s10,
    //seL4_TimeoutReply_s11,
    //seL4_TimeoutReply_a0,
    //seL4_TimeoutReply_a1,
    //seL4_TimeoutReply_a2,
    //seL4_TimeoutReply_a3,
    //seL4_TimeoutReply_a4,
    //seL4_TimeoutReply_a5,
    //seL4_TimeoutReply_a6,
    //seL4_TimeoutReply_a7,
    //seL4_TimeoutReply_t0,
    //seL4_TimeoutReply_t1,
    //seL4_TimeoutReply_t2,
    //seL4_TimeoutReply_t3,
    //seL4_TimeoutReply_t4,
    //seL4_TimeoutReply_t5,
    //seL4_TimeoutReply_t6,
    //seL4_TimeoutReply_TP,
    seL4_TimeoutReply_Length,
} seL4_TimeoutReply_Msg;

typedef enum {
    seL4_Timeout_Data,
    seL4_Timeout_Consumed,
    seL4_Timeout_Length,
} seL4_TimeoutMsg;

#endif /* CONFIG_KERNEL_MCS */

#endif /* not __ASSEMBLER__ */

/* First address in the virtual address space that is not accessible to user level */
#define seL4_UserTop 0x0000003ffffff000

#ifdef CONFIG_ENABLE_BENCHMARKS
/* size of kernel log buffer in bytes */
#define seL4_LogBufferSize (LIBSEL4_BIT(20))
#endif /* CONFIG_ENABLE_BENCHMARKS */