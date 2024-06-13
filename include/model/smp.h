/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <arch/types.h>
#include <arch/model/statedata.h>
#include <model/statedata.h>

#ifdef ENABLE_SMP_SUPPORT

typedef struct smpStatedata {
    archNodeState_t cpu;
    nodeState_t system;
    PAD_TO_NEXT_CACHE_LN(sizeof(archNodeState_t) + sizeof(nodeState_t));
} smpStatedata_t;

extern smpStatedata_t ksSMP[CONFIG_MAX_NUM_NODES];

static bool_t is_valid_core(word_t core) {
    /* Currently the affinity value in both userland and kernel is a linear core
     * IDs. The actual mapping is architecture and platform specific. We check
     * against ksNumCPUs, because this is the number of cores that booted
     * successfully. This is supposed to be equal to CONFIG_MAX_NUM_NODES, which
     * is the number of cores the kernel tries to boot.
     */
    return (core < ksNumCPUs);
}

void migrateTCB(tcb_t *tcb, word_t new_core);

#endif /* ENABLE_SMP_SUPPORT */
