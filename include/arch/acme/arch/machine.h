/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#ifndef __ASSEMBLER__
#include <arch/types.h>
#include <arch/object/structures.h>
#include <arch/machine/hardware.h>
#include <arch/model/statedata.h>
#include <mode/machine.h>

static inline void acme_data_barrier(void)
{
    /* here be dragons */
}

static inline void acme_inst_barrier(void)
{
    /* here be dragons */
}

static inline void hwASIDFlushLocal(asid_t asid)
{
    /* here be dragons */
}

static inline void hwASIDFlush(asid_t asid)
{
    hwASIDFlushLocal(asid);
    /* here be dragons */
}

word_t PURE getRestartPC(tcb_t *thread);
void setNextPC(tcb_t *thread, word_t v);

/* Cleaning memory before user-level access */
static inline void clearMemory(void *ptr, unsigned int bits)
{
    memzero(ptr, BIT(bits));
}

static inline void setVSpaceRoot(paddr_t addr, asid_t asid)
{
    satp_t satp = satp_new(SATP_MODE_SV39,         /* mode */
                           asid,                   /* asid */
                           addr >> seL4_PageBits); /* PPN */

    (void)satp.words[0]; /* dummy */

    acme_data_barrier(); /* Order read/write operations */
}

void map_kernel_devices(void);

/** MODIFIES: [*] */
void initTimer(void);
void initLocalIRQController(void);
void initIRQController(void);
void setIRQTrigger(irq_t irq, bool_t trigger);

#ifdef ENABLE_SMP_SUPPORT
#define irq_remote_call_ipi     (INTERRUPT_IPI_0)
#define irq_reschedule_ipi      (INTERRUPT_IPI_1)

static inline void arch_pause(void)
{
    /* there is no pause */
}

#endif

/* Update the value of the actual register to hold the expected value */
static inline exception_t Arch_setTLSRegister(word_t tls_base)
{
    /* The register is always reloaded upon return from kernel. */
    setRegister(NODE_STATE(ksCurThread), TLS_BASE, tls_base);
    return EXCEPTION_NONE;
}

#endif // __ASSEMBLER__
