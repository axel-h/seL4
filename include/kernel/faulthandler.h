/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <types.h>
#include <object/structures.h>

/* ToDo: seems there is no dedicated value */
#define NO_LOOKUP_FAULT lookup_fault_missing_capability_new(0)

#ifdef CONFIG_KERNEL_MCS
bool_t tryRaisingTimeoutFault(tcb_t *tptr, word_t scBadge);
#endif

void handleFault(tcb_t *tptr, seL4_Fault_t fault, lookup_fault_t lookup_fault);
