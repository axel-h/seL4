/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <arch/machine.h>
#include <arch/machine/timer.h>
#include <arch/kernel/boot_sys.h>
#include <arch/kernel/smp_sys.h>
#include <smp/lock.h>

#ifdef ENABLE_SMP_SUPPORT

#ifdef CONFIG_USE_LOGICAL_IDS
BOOT_CODE static void update_logical_id_mappings(void)
{
    cpu_id_t idx = getCurrentCPUIndex();
    logical_id_t id = apic_get_logical_id();

    cpu_mapping.index_to_logical_id[idx] = id;

    for (int i = 0; i < ksNumCPUs; i++) {
        if (id == apic_get_cluster(cpu_mapping.index_to_logical_id[i])) {
            cpu_mapping.other_indexes_in_cluster[idx] |= BIT(i);
            cpu_mapping.other_indexes_in_cluster[i] |= BIT(idx);
        }
    }}
#endif /* CONFIG_USE_LOGICAL_IDS */

BOOT_CODE static void start_cpu(cpu_id_t cpu_id, paddr_t boot_fun_paddr)
{
    /* memory fence needed before starting the other CPU */
    x86_mfence();

    /* starting the other CPU */
    apic_send_init_ipi(cpu_id);
    apic_send_startup_ipi(cpu_id, boot_fun_paddr);
}

BOOT_CODE void release_secondary_cores(void)
{
    /* update cpu mapping for BSP, cpus[0] is always assumed to be BSP */
    cpu_mapping.index_to_cpu_id[getCurrentCPUIndex()] = boot_state.cpus[0];
#ifdef CONFIG_USE_LOGICAL_IDS
    cpu_mapping.index_to_logical_id[getCurrentCPUIndex()] = apic_get_logical_id();
#endif /* CONFIG_USE_LOGICAL_IDS */

    /* startup APs one at a time as we use shared kernel boot stack */
    while (ksNumCPUs < boot_state.num_cpus) {
        word_t current_ap_index = ksNumCPUs;
        cpu_id_t id = boot_state.cpus[current_ap_index]

        printf("Starting node #%lu with APIC ID %lu \n",
               current_ap_index, id);

        /* update cpu mapping for APs, store APIC ID of the next booting AP
         * as APIC ID are not continoius e.g. 0,2,1,3 for 4 cores with hyperthreading
         * we need to store a mapping to translate the index to real APIC ID */
        cpu_mapping.index_to_cpu_id[current_ap_index] = id;
        SMP_CLOCK_SYNC_TEST_UPDATE_TIME();
        start_cpu(id, BOOT_NODE_PADDR);

        /* wait for current AP to boot up */
        while (smp_aps_index == current_ap_index) {
            SMP_CLOCK_SYNC_TEST_UPDATE_TIME();
        }
    }
}

BOOT_CODE bool_t copy_boot_code_aps(uint32_t mem_lower)
{
    assert(boot_cpu_end - boot_cpu_start < 0x400);

    /* Ensure that our boot code fits in the memory hole we want to use, and check this region
     * is free according to multiboot. As boot_cpu_end and boot_cpu_start are link time
     * symbols (and not compile time) this cannot be a compile time check */
    word_t boot_size = (word_t)(boot_cpu_end - boot_cpu_start);
    word_t boot_node_top = BOOT_NODE_PADDR + boot_size;
    word_t mem_lower_bytes = mem_lower << 10;
    if (boot_node_top > BOOT_NODE_MAX_PADDR) {
        printf("AP boot code does not fit in chosen memory hole. Can be at most %lu, is %lu\n",
               (word_t)(BOOT_NODE_MAX_PADDR - BOOT_NODE_PADDR), boot_size);
        return false;
    }
    if (mem_lower_bytes < boot_node_top) {
        printf("Need lower physical memory up to %lu to be free. Multiboot reports only up to %lu\n",
               boot_node_top, mem_lower_bytes);
        return false;
    }

    /* copy CPU bootup code to lower memory */
    memcpy((void *)BOOT_NODE_PADDR, boot_cpu_start, boot_size);
    return true;
}

/* This is the entry function for APs. However, it is not a BOOT_CODE as
 * there is a race between exiting this function and root task running on
 * node #0 to possibly reallocate this memory */
VISIBLE void boot_node(void)
{
    mode_init_tls(ksNumCPUs);

    setCurrentVSpaceRoot(kpptr_to_paddr(X86_KERNEL_VSPACE_ROOT), 0);
    /* Sync up the compilers view of the world here to force the PD to actually
     * be set *right now* instead of delayed */
    x86_mfence();

    /* initialise the CPU, make sure legacy interrupts are disabled */
    if (!init_cpu(1)) {
        fail("init_cpu failed\n");
    }

#ifdef CONFIG_USE_LOGICAL_IDS
    update_logical_id_mappings();
#endif /* CONFIG_USE_LOGICAL_IDS */

    if (!finalize_init_kernel_on_secondary_core()) {
        fail("finalize_init_kernel_on_secondary_core failed for some reason :(\n");
    }
}

#endif /* ENABLE_SMP_SUPPORT */
