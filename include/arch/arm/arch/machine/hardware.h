/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <mode/machine/hardware.h>

#ifndef __ASSEMBLER__
enum vm_fault_type {
    ARMDataAbort = seL4_DataFault,
    ARMPrefetchAbort = seL4_InstructionFault
};
typedef word_t vm_fault_type_t;

#define PAGE_BASE(_p, _s)        ((_p) & ~MASK(pageBitsForSize((_s))))
#define PAGE_OFFSET(_p, _s)      ((_p) & MASK(pageBitsForSize((_s))))

#endif /* __ASSEMBLER__ */

#define L1_CACHE_LINE_SIZE_BITS CONFIG_L1_CACHE_LINE_SIZE_BITS
#define L1_CACHE_LINE_SIZE BIT(L1_CACHE_LINE_SIZE_BITS)
