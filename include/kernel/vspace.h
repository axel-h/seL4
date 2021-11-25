/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once

#include <config.h>
#include <arch/kernel/vspace.h>

#ifdef CONFIG_KERNEL_LOG_BUFFER
exception_t benchmark_arch_map_logBuffer(word_t frame_cptr);
#endif /* CONFIG_KERNEL_LOG_BUFFER */

#ifdef CONFIG_PRINTING

/* The function readWordFromVSpace() is used for stack dumps, thus it exists
 * only if CONFIG_PRINTING is set, regardless of CONFIG_DEBUG.
 */

typedef enum {
    VSPACE_ACCESS_SUCCESSFUL        = 0,
    VSPACE_INVALID_ROOT             = 1,
    VSPACE_LOOKUP_FAILED            = 2,
    VSPACE_INVALID_ALIGNMENT        = 3
} vspaceAccessResult_t;

typedef struct readWordFromVSpace_ret {
    vspaceAccessResult_t  status;
    word_t                value;
    word_t                paddr;
} readWordFromVSpace_ret_t;


/* Obviously, vaddr must be word-aligned for this to work. */
readWordFromVSpace_ret_t Arch_readWordFromVSpace(vspace_root_t *vspace,
                                                 word_t vaddr);

typedef struct readWordFromStack_ret {
    vspaceAccessResult_t  status;
    word_t                value;
    /* All architectures supported so far have the stack in the vspace. */
    vspace_root_t         *vspace_root;
    word_t                vaddr;
    word_t                paddr;
} readWordFromStack_ret_t;

/* It is architecture dependent what a thread stack is actually, and how to read
 * a word from it. The common mode is that there is a stack pointer that points
 * somewhere into  the thread's vspace. It is least word aligned, so accessing
 * the stack falls down to calling Arch_readWordFromVSpace() then.
 */
readWordFromStack_ret_t Arch_readWordFromThreadStack(tcb_t *tptr, word_t i);

#endif /* CONFIG_PRINTING */
