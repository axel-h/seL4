/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <basic_types.h>
#include <hardware.h>

typedef struct region {
    pptr_t start;
    pptr_t end;
} region_t;

#define REG_EMPTY ((region_t){ .start = 0, .end = 0 })

typedef struct p_region {
    paddr_t start;
    paddr_t end;
} p_region_t;

#define P_REG_EMPTY ((p_region_t){ .start = 0, .end = 0 })

typedef struct v_region {
    vptr_t start;
    vptr_t end;
} v_region_t;

/* When translating a physical address into an address accessible to the
 * kernel via virtual addressing we always use the mapping of the memory
 * into the physical memory window, even if the mapping originally
 * referred to a kernel virtual address. */
static inline void *CONST ptrFromPAddr(paddr_t paddr)
{
    return (void *)(paddr + PPTR_BASE_OFFSET);
}

/* When obtaining a physical address from a reference to any object in
 * the physical mapping window, this function must be used. */
static inline paddr_t CONST addrFromPPtr(const void *pptr)
{
    return (paddr_t)pptr - PPTR_BASE_OFFSET;
}

/* When obtaining a physical address from a reference to an address from
 * the kernel ELF mapping, this function must be used. */
static inline paddr_t CONST addrFromKPPtr(const void *pptr)
{
    assert((paddr_t)pptr >= KERNEL_ELF_BASE);
    assert((paddr_t)pptr <= KERNEL_ELF_TOP);
    return (paddr_t)pptr - KERNEL_ELF_BASE_OFFSET;
}

static inline region_t CONST paddr_to_pptr_reg(const p_region_t p_reg)
{
    return (region_t) {
        p_reg.start + PPTR_BASE_OFFSET, p_reg.end + PPTR_BASE_OFFSET
    };
}

static inline p_region_t CONST pptr_to_paddr_reg(const region_t reg)
{
    return (p_region_t) {
        reg.start - PPTR_BASE_OFFSET, reg.end - PPTR_BASE_OFFSET
    };
}

#define paddr_to_pptr(x)   ptrFromPAddr(x)
#define pptr_to_paddr(x)   addrFromPPtr(x)
#define kpptr_to_paddr(x)  addrFromKPPtr(x)
