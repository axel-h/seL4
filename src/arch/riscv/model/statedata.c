/*
 * Copyright 2020, DornerWorks
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <util.h>
#include <api/types.h>
#include <arch/types.h>
#include <arch/model/statedata.h>
#include <arch/object/structures.h>
#include <linker.h>
#include <plat/machine/hardware.h>

/* The top level asid mapping table */
asid_pool_t *riscvKSASIDTable[BIT(asidHighBits)];

/* Kernel Page Tables */
pte_t kernel_root_pageTable[BIT(PT_INDEX_BITS)] ALIGN_BSS(BIT(seL4_PageTableBits));

#if __riscv_xlen != 32
pte_t kernel_image_level2_pt[BIT(PT_INDEX_BITS)] ALIGN_BSS(BIT(seL4_PageTableBits));
pte_t kernel_image_level2_dev_pt[BIT(PT_INDEX_BITS)] ALIGN_BSS(BIT(seL4_PageTableBits));
#elif defined(CONFIG_KERNEL_LOG_BUFFER)
pte_t kernel_image_level2_log_buffer_pt[BIT(PT_INDEX_BITS)] ALIGN_BSS(BIT(seL4_PageTableBits));
#endif

/* Unlike on ARM, there is no register in RISC-V that allows reading the current
 * hart ID. It is passed during boot and thus we have to remember it even in
 * non-SMP configurations.
 */
core_map_t coreMap;


word_t get_current_hart_id(void)
{
#ifdef CONFIG_ENABLE_SMP_SUPPORT
    return cpuIndexToID(getCurrentCPUIndex());
#else
    return coreMap.cores[0].hart_id;
#endif
}
