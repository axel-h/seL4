/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <benchmark/benchmark.h>

#ifdef ENABLE_TRACE_KERNEL_ENTRY

#include <sel4/benchmark_track_types.h>
#include <sel4/arch/constants.h>
#include <machine/io.h>
#include <kernel/cspace.h>
#include <model/statedata.h>
#include <mode/machine.h>

static inline void benchmark_debug_syscall_start(word_t cptr, word_t msgInfo, word_t syscall)
{
    seL4_MessageInfo_t info = messageInfoFromWord_raw(msgInfo);
    lookupCapAndSlot_ret_t lu_ret = lookupCapAndSlot(NODE_STATE(ksCurThread), cptr);
    ksKernelEntry.path = Entry_Syscall;
    ksKernelEntry.syscall_no = -syscall;
    ksKernelEntry.cap_type = cap_get_capType(lu_ret.cap);
    ksKernelEntry.invocation_tag = seL4_MessageInfo_get_label(info);
}

#endif /* ENABLE_TRACE_KERNEL_ENTRY_EXIT */
