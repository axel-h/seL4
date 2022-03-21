/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <util.h>
#include <mode/hardware.h>

#define LOAD_S  STRINGIFY(LOAD)
#define STORE_S STRINGIFY(STORE)

#ifndef __ASSEMBLER__

#include <linker.h>
#include <arch/types.h>
#include <sel4/sel4_arch/constants.h>

#define L1_CACHE_LINE_SIZE_BITS     6 /* 64 byte */
#define L1_CACHE_LINE_SIZE          BIT(L1_CACHE_LINE_SIZE_BITS)

#define PAGE_BITS seL4_PageBits

#define RISCV_GET_PT_INDEX(addr, n)  (((addr) >> (((PT_INDEX_BITS) * (((CONFIG_PT_LEVELS) - 1) - (n))) + seL4_PageBits)) & MASK(PT_INDEX_BITS))
#define RISCV_GET_LVL_PGSIZE_BITS(n) (((PT_INDEX_BITS) * (((CONFIG_PT_LEVELS) - 1) - (n))) + seL4_PageBits)
#define RISCV_GET_LVL_PGSIZE(n)      BIT(RISCV_GET_LVL_PGSIZE_BITS((n)))

enum vm_fault_type {
    ACMEFault = 0,
};
typedef word_t vm_fault_type_t;

enum frameSizeConstants {
    ACMEPageBits        = seL4_PageBits,
    ACMEMegaPageBits    = seL4_LargePageBits,
    ACMEGigaPageBits    = seL4_HugePageBits,
    ACMETeraPageBits    = seL4_TeraPageBits
};

enum vm_page_size {
    ACME_4K_Page,
    ACME_Mega_Page,
    ACME_Giga_Page,
    ACME_Tera_Page
};
typedef word_t vm_page_size_t;

static inline word_t CONST pageBitsForSize(vm_page_size_t pagesize)
{
    switch (pagesize) {
    case ACME_4K_Page:
        return ACMEPageBits;

    case RISCV_Mega_Page:
        return ACMEMegaPageBits;

    case RISCV_Giga_Page:
        return ACMEGigaPageBits;

    default:
        fail("Invalid page size");
    }
}

static inline void arch_clean_invalidate_caches(void)
{
    /* here be dragons */
}

#define IPI_MEM_BARRIER do { } while (0)

#endif /* not __ASSEMBLER__ */

