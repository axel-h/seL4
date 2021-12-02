/* SPDX-License-Identifier: BSD-3-Clause */

/* Copyright (c) 2010-2017, The Regents of the University of California
 * (Regents).  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Regents nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 * BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 * HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/* This file was derived from RISC-V tools/linux project */

#pragma once

#include <config.h>
#include <stdint.h>

/* The RISC-V Supervisor Binary Interface (SBI) specification is available at
 * https://github.com/riscv-non-isa/riscv-sbi-doc and covers both the legacy
 * interface and the new extensible interface. We have to support platform that
 * just implement the legacy interface and it is sufficient what the kernel
 * needs, thus the SBI wrappers below keep using it.
 */
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8
/* The values 9 - 15 are reserved. */

typedef struct {
    word_t error;
    word_t value;
} sbiret_t;

static inline sbiret_t sbi_call(word_t extension_id,
                                word_t function_id,
                                word_t arg_0,
                                word_t arg_1,
                                word_t arg_2,
                                word_t arg_3)
{
    register word_t a0 asm("a0") = arg_0;
    register word_t a1 asm("a1") = arg_1;
    register word_t a2 asm("a2") = arg_2;
    register word_t a3 asm("a3") = arg_3;
    register word_t a6 asm("a6") = function_id;
    register word_t a7 asm("a7") = extension_id;

    /* Seems the proofs don't support "+r" for inline assembly to declare that
     * a variable (actually a register) is modified. Only "=r" is supported, so
     * there are strict separation between input and output variables.
     */
    register word_t a0_ret asm("a0");
    register word_t a1_ret asm("a1");

    asm volatile("ecall"
                 : "=r"(a0_ret), "=r"(a1_ret)
                 : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a6), "r"(a7)
                 : "memory");

    return (sbiret_t) {
        .error = a0_ret,
        .value = a1_ret
    };
}

static inline word_t sbi_call_legacy(word_t cmd,
                                     word_t arg_0,
                                     word_t arg_1,
                                     word_t arg_2,
                                     word_t arg_3)
{
    /* The new interface was designed as an extension of the legacy interface,
     * the command became the extension ID and the function ID was added as a
     * new parameter, which legacy calls ignore.
     */
    sbiret_t sbiret = sbi_call(cmd, 0, arg_0, arg_1, arg_2, arg_3);
    return sbiret.error;
}


static inline void sbi_console_putchar(int ch)
{
    (void)sbi_call_legacy(SBI_CONSOLE_PUTCHAR, ch, 0, 0, 0);
}

static inline int sbi_console_getchar(void)
{
    return (int)sbi_call_legacy(SBI_CONSOLE_GETCHAR, 0, 0, 0, 0);
}

static inline void sbi_set_timer(uint64_t timestamp)
{
#if defined(CONFIG_ARCH_RISCV32)
    word_t timestamp_hi = (word_t)timestamp;
    word_t timestamp_lo = (word_t)(timestamp >> 32);
    (void)sbi_call_legacy(SBI_SET_TIMER, timestamp_hi, timestamp_lo, 0, 0);
#elif defined(CONFIG_ARCH_RISCV64)
    (void)sbi_call_legacy(SBI_SET_TIMER, timestamp, 0, 0, 0);
#else
#error "unsupported architecture"
#endif
}

static inline void sbi_shutdown(void)
{
    (void)sbi_call_legacy(SBI_SHUTDOWN, 0, 0, 0, 0);
}

#ifdef ENABLE_SMP_SUPPORT

static inline void sbi_clear_ipi(void)
{
    (void)sbi_call_legacy(SBI_CLEAR_IPI, 0, 0, 0, 0);
}

static inline void sbi_send_ipi(word_t hart_mask)
{
    /* ToDo: In the legacy SBI API the hart mask parameter is not a value, but
     *       the virtual address of a bit vector. This was intended to allow
     *       passing an arbitrary number of harts without being limited by the
     *       architecture's word size. We hide this feature here because:
     *       - All systems currently supported by the kernel have less harts
     *           than bits in a word. Support for more harts would requires
     *           reworking parts of the kernel.
     *       - The legacy SBI interface is deprecated. Passing pointers with
     *           virtual addresses has several practical drawbacks or corner
     *           cases, these outweight the gain of being able to address all
     *           harts in one call. The new interface uses a window concept and
     *           passes plain values.
     *       - Using pointers to local variables is perfectly fine in C, but
     *           verification does not support this, because passing pointers to
     *           stack objects is not allowed. However, verification does not
     *           run for the RISC-V SMP configuration at the moment, thus
     *           keeping this detail hidden here allows postponing finding a
     *           solution and higher layers are kept agnostic of this.
     */
    word_t virt_addr_hart_mask = (word_t)&hart_mask;
    (void)sbi_call_legacy(SBI_SEND_IPI, virt_addr_hart_mask, 0, 0, 0);
}

static inline void sbi_remote_fence_i(word_t hart_mask)
{
    /* See comment at sbi_send_ipi() about the pointer parameter. */
    word_t virt_addr_hart_mask = (word_t)&hart_mask;
    (void)sbi_call_legacy(SBI_REMOTE_FENCE_I, virt_addr_hart_mask, 0, 0, 0);
}

static inline void sbi_remote_sfence_vma(word_t hart_mask,
                                         word_t start,
                                         word_t size)
{
    /* See comment at sbi_send_ipi() about the pointer parameter. */
    word_t virt_addr_hart_mask = (word_t)&hart_mask;
    (void)sbi_call_legacy(SBI_REMOTE_SFENCE_VMA, virt_addr_hart_mask, start,
                          size, 0);
}

static inline void sbi_remote_sfence_vma_asid(word_t hart_mask,
                                              word_t start,
                                              word_t size,
                                              word_t asid)
{
    /* See comment at sbi_send_ipi() about the pointer parameter. */
    word_t virt_addr_hart_mask = (word_t)&hart_mask;
    (void)sbi_call_legacy(SBI_REMOTE_SFENCE_VMA_ASID, virt_addr_hart_mask,
                          start, size, asid);
}

#endif /* ENABLE_SMP_SUPPORT */
