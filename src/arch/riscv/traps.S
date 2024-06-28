/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <machine/assembler.h>
#include <arch/machine/hardware.h>
#include <arch/api/syscall.h>
#include <arch/machine/registerset.h>
#include <util.h>

#define REGBYTES (CONFIG_WORD_SIZE / 8)

.extern c_handle_syscall
.extern c_handle_fastpath_reply_recv
.extern c_handle_fastpath_call
.extern c_handle_interrupt
.extern c_handle_exception

.section .text.traps, "ax"
BEGIN_FUNC(trap_entry)

  /* The general concept here is loading tp/x4 with a pointer to the space where
   * to store the user space context.
   */
#ifdef ENABLE_SMP_SUPPORT
  /* SSCRATCH holds the kernel stack pointer to be used. The first item on the
   * stack is a pointer to the space where to store the thread context.
   */
  csrrw sp, sscratch, sp        /* swap sp/x1 and sscratch to get the kernel stack */
  STORE tp, (-2*REGBYTES)(sp)   /* sp[-2] = tp/x4 (temporarily save tp/x4 on kernel stack) */
  LOAD  tp, (-1*REGBYTES)(sp)   /* tp = sp[-1] (get thread context space) */
  STORE ra, (0*REGBYTES)(tp)    /* tp[0] = ra/x1 */
  csrrw ra, sscratch, sp        /* ra/x1 <- sscratch (user's sp/x2) <- sp/x1 */
  STORE ra, (1*REGBYTES)(tp)    /* tp[1] = user sp/x2 (stored temporaily in ra/x1) */
  LOAD  ra, (-2*REGBYTES)(sp)   /* ra/x1 = user's tp/x4 (stored temporaily in sp[-2] */
#else
  /* SSCRATCH holds a pointer to the space where to store the thread context. */
  csrrw tp, sscratch, tp        /* swap tp/x4 and sscratch */
  STORE ra, (0*REGBYTES)(tp)    /* tp[0] = ra/x1 */
  STORE sp, (1*REGBYTES)(tp)    /* tp[1] = sp/x2 */
  csrrw ra, sscratch, tp        /* ra/x1 <- sscratch (user's tp/x4) <- tp/x4 */
  la sp, (kernel_stack_alloc + BIT(CONFIG_KERNEL_STACK_BITS))
#endif
  /* x1/ra and x2/sp have already been saved */
  STORE gp, (2*REGBYTES)(tp)    /* tp[2]  = gp/x3 */
  STORE ra, (3*REGBYTES)(tp)    /* tp[3]  = user's tp/x4 has been stored in ra/x1 */
  STORE t0, (4*REGBYTES)(tp)    /* tp[4]  = t0/x5 */
  STORE t1, (5*REGBYTES)(tp)    /* tp[5]  = t1/x6 */
  STORE t2, (6*REGBYTES)(tp)    /* tp[6]  = t2/x7 */
  STORE s0, (7*REGBYTES)(tp)    /* tp[7]  = s0/x8 */
  STORE s1, (8*REGBYTES)(tp)    /* tp[8]  = s1/x9 */
  STORE a0, (9*REGBYTES)(tp)    /* tp[9]  = a0/x10 */
  STORE a1, (10*REGBYTES)(tp)   /* tp[10] = a1/x11*/
  STORE a2, (11*REGBYTES)(tp)   /* tp[11] = a2/x12 */
  STORE a3, (12*REGBYTES)(tp)   /* tp[12] = a3/x13 */
  STORE a4, (13*REGBYTES)(tp)   /* tp[13] = a4/x14 */
  STORE a5, (14*REGBYTES)(tp)   /* tp[14] = a5/x15 */
  STORE a6, (15*REGBYTES)(tp)   /* tp[15] = a6/x16 */
  STORE a7, (16*REGBYTES)(tp)   /* tp[16] = a7/x17 */
  STORE s2, (17*REGBYTES)(tp)   /* tp[17] = s2/x18 */
  STORE s3, (18*REGBYTES)(tp)   /* tp[18] = s3/x19 */
  STORE s4, (19*REGBYTES)(tp)   /* tp[19] = s4/x20 */
  STORE s5, (20*REGBYTES)(tp)   /* tp[20] = s5/x21 */
  STORE s6, (21*REGBYTES)(tp)   /* tp[21] = s6/x22 */
  STORE s7, (22*REGBYTES)(tp)   /* tp[22] = s7/x23 */
  STORE s8, (23*REGBYTES)(tp)   /* tp[23] = s8/x24 */
  STORE s9, (24*REGBYTES)(tp)   /* tp[24] = s9/x25 */
  STORE s10, (25*REGBYTES)(tp)  /* tp[25] = s10/x26 */
  STORE s11, (26*REGBYTES)(tp)  /* tp[26] = s11/x27 */
  STORE t3, (27*REGBYTES)(tp)   /* tp[27] = t3/x28 */
  STORE t4, (28*REGBYTES)(tp)   /* tp[28] = t4/x29 */
  STORE t5, (29*REGBYTES)(tp)   /* tp[29] = t5/x30 */
  STORE t6, (30*REGBYTES)(tp)   /* tp[30] = t6/x31 */

  csrr  t0, scause
  STORE t0, (31*REGBYTES)(tp)   /* tp[31] = scause */
  csrr  t1, sstatus
  STORE t1, (32*REGBYTES)(tp)   /* tp[32] = sstatus */
  csrr  t2, sepc
  STORE t2, (33*REGBYTES)(tp)   /* tp[33] = sepc */

.option push
.option norelax
  la gp, __global_pointer$
.option pop

  /* Check if it's an interrupt (t0 holds scause, MSB set indicates interrupt) */
  bltz t0, handle_interrupt

  /* ratified priv has value 8 for ecall from U-mode exception */
  li   t3, 8
  bne  t0, t3, handle_exception

handle_syscall:
  /* Set the return address (t2 holds sepc) to +4 in the case of a system/environment call */
  addi t2, t2, 4
  STORE t2, (34*REGBYTES)(tp)   /* tp[34]/NextIP = sepc + 4 (t2 holds sepc) */

#ifdef CONFIG_FASTPATH
  li t3, SYSCALL_CALL
  bne a7, t3, test_replyrecv
  j c_handle_fastpath_call

test_replyrecv:
  li t3, SYSCALL_REPLY_RECV
#ifdef CONFIG_KERNEL_MCS
  /* move reply to 3rd argument */
  mv a2, a6
#endif
  bne a7, t3, slowpath
  j c_handle_fastpath_reply_recv

slowpath:
#endif

  /* move syscall number to 3rd argument */
  mv a2, a7

  j c_handle_syscall

/* Not an interrupt or a syscall */
handle_exception:
  STORE t2, (34*REGBYTES)(tp)   /* tp[34]/NextIP = sepc (stored in t2) */
  j c_handle_exception

handle_interrupt:
  STORE t2, (34*REGBYTES)(tp)   /* tp[34]/NextIP = sepc (stored in t2) */
  j c_handle_interrupt

END_FUNC(trap_entry)
