/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <types.h>
#include <util.h>
#include <arch/model/smp.h>
#include <mode/smp/smp.h>

#ifdef CONFIG_ENABLE_SMP_SUPPORT

void migrateTCB(tcb_t *tcb, word_t new_core);
static inline CONST cpu_id_t getCurrentCPUIndex(void);

static int init_tcb_on_current_core(tcb_t *tcb) {
    assert(tcb);
    tcb->tcbAffinity = getCurrentCPUIndex()
}

static int is_tcb_on_current_core(tcb_t *tcb) {
    assert(tcb);
    return (tcb->tcbAffinity == getCurrentCPUIndex());
}

#endif /* [not] CONFIG_ENABLE_SMP_SUPPORT */

#define CURRENT_CPU_INDEX() \
    SMP_TERNARY(getCurrentCPUIndex(), SEL4_WORD_CONST(0))

static inline bool_t isCurrentCore(word_t core)
{
    return SMP_TERNARY(core == CURRENT_CPU_INDEX(), true)
}
