/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h> /* included by convention, regardless of dependency */
#include <basic_types.h> /* included by convention, regardless of dependency */
#include <compound_types.h> /* included by convention, regardless of dependency */

/* for libsel4 headers that the kernel shares */
typedef word_t seL4_Word;
typedef uint64_t seL4_Uint64;
typedef uint32_t seL4_Uint32;
typedef uint16_t seL4_Uint16;
typedef uint8_t seL4_Uint8;
typedef cptr_t seL4_CPtr;
typedef node_id_t seL4_NodeId;
typedef paddr_t seL4_PAddr;
typedef dom_t seL4_Domain;
