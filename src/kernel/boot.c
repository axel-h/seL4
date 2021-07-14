/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <assert.h>
#include <kernel/boot.h>
#include <kernel/thread.h>
#include <machine/io.h>
#include <machine/registerset.h>
#include <model/statedata.h>
#include <arch/machine.h>
#include <arch/kernel/boot.h>
#include <arch/kernel/vspace.h>
#include <linker.h>
#include <hardware.h>
#include <util.h>

/* (node-local) state accessed only during bootstrapping */
BOOT_BSS ndks_boot_t ndks_boot;

BOOT_BSS rootserver_mem_t rootserver;
BOOT_BSS static region_t rootserver_mem;

static bool_t print_active = 0;
BOOT_CODE void hack_enable_prints(void) { print_active = 1; }

BOOT_CODE bool_t reserve_region(p_region_t reg)
{
    if (print_active) {
        printf("reserve region [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
               reg.start, reg.end - 1);
    }

    if (reg.start > reg.end) {
        printf("ERROR: invalid region\n");
        return false;
    }

    /* region must be sane */
    assert(reg.start <= reg.end);

    /* There is noting to do if the region has a zero size. */
    if (reg.start == reg.end) {
        printf("  nothing to do for empty regions\n");
        return true;
    }

    /* There is noting to do if the region has a zero size. */
    if (reg.start == reg.end) {
        return true;
    }

    /* The list of regions is ordered properly and no regions are overlapping
     * or adjacent. Check where to insert the current region or if we can merge
     * it with one or more regions.
     */
    word_t i;
    for (i = 0; i < ndks_boot.resv_count; i++) {
        p_region_t *cur_reg = &ndks_boot.reserved[i];

        if (print_active) {
            printf("  %d  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
                   (int)i, cur_reg->start, cur_reg->end - 1);
        }

        if (reg.start > cur_reg->end) {
            /* The list or properly ordered, so there is no impact on the
             * current element if new region is after it.
             *
             *                   |--reg--|
             *    |--cur_reg--|
             */
            if (print_active) {
               printf("    skip\n");
            }
            continue;
        }

        if (reg.end < cur_reg->start) {
            /* The list or properly ordered, if the new element is before the
             * current element then we have to make space and insert it.
             *
             *    |--reg--|
             *               |--cur_reg--|
             */
            if (print_active) {
               printf("    insert before\n");
            }
            break;
        }

        /* For all remain cases, the current region is merged with the new one.
         *
         * case 1a:  |--reg--|
         *           |<------|--cur_reg--|
         *
         * case 1b:      |--reg--|
         *               |<--|--cur_reg--|
         *
         * case 1c:      |--reg----------|
         *               |<--|--cur_reg--|
         *
         * case 2a:          |----------reg--|
         *                   |--cur_reg--|-->|
         *
         * case 2b:                  |--reg--|
         *                   |--cur_reg--|-->|
         *
         * case 2c:                      |--reg--|
         *                   |--cur_reg--|------>|
         *
         * case 3a:            |--reg--|
         *                   |--cur_reg--|
         *
         * case 3b:  |------------reg------------|
         *           |<------|--cur_reg--|------>|
         *
         * For case 3a there is nothing to do. Case 3b is actually a combination
         * of case 1b and 2b
         *
         */
        if (print_active) {
            printf("    merge\n");
        }
        if (reg.start < cur_reg->start) {
            /* Adjust the region start if the new region starts earlier. */
            cur_reg->start = reg.start;
        }

        if (reg.end > cur_reg->end) {
            /* Adjust the region end if the new region end later. However, the
             * new region could spawn more than just the current region, so we
             * may have to merge more regions.
             */
            cur_reg->end = reg.end;
            word_t cnt = 0;
            i++;
            while (i + cnt < ndks_boot.resv_count) {
                cur_reg = &ndks_boot.reserved[i + cnt];
                if (print_active) {
                    printf("    merge %d  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
                           (int)(i + cnt), cur_reg->start, cur_reg->end - 1);
                }
                cnt++;
                if (reg.end < cur_reg->start) {
                    break;
                }
            }

            if (cnt > 0)
            {
                /* Move regions to close the gap. */
                for ( /*nothing */; i + cnt < ndks_boot.resv_count; i++) {
                    if (print_active) {
                        printf("    move %d -> %d  "
                               "[%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
                               (int)(i + cnt), (int)i, cur_reg->start,
                               cur_reg->end  - 1);
                        }
                    ndks_boot.reserved[i] = ndks_boot.reserved[i + cnt];
                }

                assert( i == ndks_boot.resv_count - cnt );

                /* Mark remaining regions as empty. */
                for ( /*nothing */; i < ndks_boot.resv_count; i++) {
                    if (print_active) {
                        printf("    clear %d\n", (int)i);
                    }
                    ndks_boot.reserved[i] = P_REG_EMPTY;
                }

                ndks_boot.resv_count -= cnt;
            }
        }

        return true;
    }

    /* Append at the end or make space to insert - if there is space. */
    if (i >= MAX_NUM_RESV_REG) {
        printf("ERROR: can't reserve [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"] "
               "as it execeeds MAX_NUM_RESV_REG (%d)\n",
               reg.start, reg.end - 1, (int)MAX_NUM_RESV_REG);

        return false;
    }

    /* If we are inserting then we have to make space first */
    for (word_t j = ndks_boot.resv_count; j > i; j--) {
        ndks_boot.reserved[j] = ndks_boot.reserved[j-1];
        if (print_active) {
            printf("    move %d -> %d "[%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
                   (int)(j-1), (int)j, cur_reg->start, cur_reg->end - 1);
        }
    }
    /* insert or append region  */
    if (print_active) {
        printf("    put at %d\n", (int)i);
    }
    ndks_boot.reserved[i] = reg;
    ndks_boot.resv_count++;
    return true;
}

BOOT_CODE static bool_t insert_region(p_region_t p_reg)
{
    assert(p_reg.start <= p_reg.end);
    if (p_reg.start == p_reg.end) {
        return true;
    }

    /* Add region to the list of reserved regions. */
    if (!reserve_region(p_reg))
    {
        printf("ERROR: can't reserve region\n");
        return false;
    }

    /* The kernel maps the physical memory completely in it's VA space starting
     * at PPTR_BASE. However due VA space limitations, anything in the physical
     * address space above PADDR_TOP cannot be mapped. On 64-bit platforms
     * this is usually not a practical limitation, but on 32-bit platform it
     * can happen that the top part of the physical address space is not
     * accessible via the kernel window mapping. */
    if (p_reg.start >= PADDR_TOP)
    {
        /* This area is completely out or reach. */
        printf("  can't add region after PADDR_TOP (%"SEL4_PRIx_word")\n",
               PADDR_TOP);
        return true;
    }

    /* Cap the area at PADDR_TOP. */
    if (p_reg.end > PADDR_TOP)
    {
        p_reg.end = PADDR_TOP;
    }

    /* Find a free slot. */
    for (word_t i = 0; i < MAX_NUM_FREEMEM_REG; i++) {
        if (is_reg_empty(ndks_boot.freemem[i])) {
            ndks_boot.freemem[i] = paddr_to_pptr_reg(p_reg);
            return true;
        }
    }

    /* MAX_NUM_FREEMEM_REG must be increased. Note that the capDL allocation
     * toolchain does not know about MAX_NUM_FREEMEM_REG, so throwing away
     * regions may prevent capDL applications from being loaded!
     */
    printf("ERROR: can't fit memory region "
           "[%"SEL4_PRIx_word"..%"SEL4_PRIx_word"], try increasing "
           "MAX_NUM_FREEMEM_REG (currently %d)\n",
           p_reg.start, p_reg.end, (int)MAX_NUM_FREEMEM_REG);

#ifdef CONFIG_ARCH_ARM
        assert(!"Ran out of freemem slots");
#endif

    return false;

}

BOOT_CODE static pptr_t alloc_rootserver_obj(word_t size_bits, word_t n)
{
    pptr_t allocated = rootserver_mem.start;
    /* allocated memory must be aligned */
    assert(allocated % BIT(size_bits) == 0);
    rootserver_mem.start += (n * BIT(size_bits));
    /* we must not have run out of memory */
    assert(rootserver_mem.start <= rootserver_mem.end);
    memzero((void *) allocated, n * BIT(size_bits));
    return allocated;
}

BOOT_CODE static word_t rootserver_max_size_bits(word_t extra_bi_size_bits)
{
    word_t cnode_size_bits = CONFIG_ROOT_CNODE_SIZE_BITS + seL4_SlotBits;
    word_t max = MAX(cnode_size_bits, seL4_VSpaceBits);
    return MAX(max, extra_bi_size_bits);
}

BOOT_CODE static word_t calculate_rootserver_size(v_region_t v_reg, word_t extra_bi_size_bits)
{
    /* work out how much memory we need for root server objects */
    word_t size = BIT(CONFIG_ROOT_CNODE_SIZE_BITS + seL4_SlotBits);
    size += BIT(seL4_TCBBits); // root thread tcb
    size += 2 * BIT(seL4_PageBits); // boot info + ipc buf
    size += BIT(seL4_ASIDPoolBits);
    size += extra_bi_size_bits > 0 ? BIT(extra_bi_size_bits) : 0;
    size += BIT(seL4_VSpaceBits); // root vspace
#ifdef CONFIG_KERNEL_MCS
    size += BIT(seL4_MinSchedContextBits); // root sched context
#endif
    /* for all archs, seL4_PageTable Bits is the size of all non top-level paging structures */
    return size + arch_get_n_paging(v_reg) * BIT(seL4_PageTableBits);
}

BOOT_CODE static void maybe_alloc_extra_bi(word_t cmp_size_bits, word_t extra_bi_size_bits)
{
    if (extra_bi_size_bits >= cmp_size_bits && rootserver.extra_bi == 0) {
        rootserver.extra_bi = alloc_rootserver_obj(extra_bi_size_bits, 1);
    }
}

BOOT_CODE void create_rootserver_objects(pptr_t start, v_region_t v_reg, word_t extra_bi_size_bits)
{
    /* the largest object the PD, the root cnode, or the extra boot info */
    word_t cnode_size_bits = CONFIG_ROOT_CNODE_SIZE_BITS + seL4_SlotBits;
    word_t max = rootserver_max_size_bits(extra_bi_size_bits);

    word_t size = calculate_rootserver_size(v_reg, extra_bi_size_bits);
    rootserver_mem.start = start;
    rootserver_mem.end = start + size;

    maybe_alloc_extra_bi(max, extra_bi_size_bits);

    /* the root cnode is at least 4k, so it could be larger or smaller than a pd. */
#if (CONFIG_ROOT_CNODE_SIZE_BITS + seL4_SlotBits) > seL4_VSpaceBits
    rootserver.cnode = alloc_rootserver_obj(cnode_size_bits, 1);
    maybe_alloc_extra_bi(seL4_VSpaceBits, extra_bi_size_bits);
    rootserver.vspace = alloc_rootserver_obj(seL4_VSpaceBits, 1);
#else
    rootserver.vspace = alloc_rootserver_obj(seL4_VSpaceBits, 1);
    maybe_alloc_extra_bi(cnode_size_bits, extra_bi_size_bits);
    rootserver.cnode = alloc_rootserver_obj(cnode_size_bits, 1);
#endif

    /* at this point we are up to creating 4k objects - which is the min size of
     * extra_bi so this is the last chance to allocate it */
    maybe_alloc_extra_bi(seL4_PageBits, extra_bi_size_bits);
    rootserver.asid_pool = alloc_rootserver_obj(seL4_ASIDPoolBits, 1);
    rootserver.ipc_buf = alloc_rootserver_obj(seL4_PageBits, 1);
    rootserver.boot_info = alloc_rootserver_obj(seL4_PageBits, 1);

    /* TCBs on aarch32 can be larger than page tables in certain configs */
#if seL4_TCBBits >= seL4_PageTableBits
    rootserver.tcb = alloc_rootserver_obj(seL4_TCBBits, 1);
#endif

    /* paging structures are 4k on every arch except aarch32 (1k) */
    word_t n = arch_get_n_paging(v_reg);
    rootserver.paging.start = alloc_rootserver_obj(seL4_PageTableBits, n);
    rootserver.paging.end = rootserver.paging.start + n * BIT(seL4_PageTableBits);

    /* for most archs, TCBs are smaller than page tables */
#if seL4_TCBBits < seL4_PageTableBits
    rootserver.tcb = alloc_rootserver_obj(seL4_TCBBits, 1);
#endif

#ifdef CONFIG_KERNEL_MCS
    rootserver.sc = alloc_rootserver_obj(seL4_MinSchedContextBits, 1);
#endif
    /* we should have allocated all our memory */
    assert(rootserver_mem.start == rootserver_mem.end);
}

BOOT_CODE void write_slot(slot_ptr_t slot_ptr, cap_t cap)
{
    slot_ptr->cap = cap;

    slot_ptr->cteMDBNode = nullMDBNode;
    mdb_node_ptr_set_mdbRevocable(&slot_ptr->cteMDBNode, true);
    mdb_node_ptr_set_mdbFirstBadged(&slot_ptr->cteMDBNode, true);
}

/* Our root CNode needs to be able to fit all the initial caps and not
 * cover all of memory.
 */
compile_assert(root_cnode_size_valid,
               CONFIG_ROOT_CNODE_SIZE_BITS < 32 - seL4_SlotBits &&
               BIT(CONFIG_ROOT_CNODE_SIZE_BITS) >= seL4_NumInitialCaps &&
               BIT(CONFIG_ROOT_CNODE_SIZE_BITS) >= (seL4_PageBits - seL4_SlotBits))

BOOT_CODE cap_t
create_root_cnode(void)
{
    /* write the number of root CNode slots to global state */
    ndks_boot.slot_pos_max = BIT(CONFIG_ROOT_CNODE_SIZE_BITS);

    cap_t cap =
        cap_cnode_cap_new(
            CONFIG_ROOT_CNODE_SIZE_BITS,      /* radix      */
            wordBits - CONFIG_ROOT_CNODE_SIZE_BITS, /* guard size */
            0,                                /* guard      */
            rootserver.cnode              /* pptr       */
        );

    /* write the root CNode cap into the root CNode */
    write_slot(SLOT_PTR(rootserver.cnode, seL4_CapInitThreadCNode), cap);

    return cap;
}

/* Check domain scheduler assumptions. */
compile_assert(num_domains_valid,
               CONFIG_NUM_DOMAINS >= 1 && CONFIG_NUM_DOMAINS <= 256)
compile_assert(num_priorities_valid,
               CONFIG_NUM_PRIORITIES >= 1 && CONFIG_NUM_PRIORITIES <= 256)

BOOT_CODE void
create_domain_cap(cap_t root_cnode_cap)
{
    /* Check domain scheduler assumptions. */
    assert(ksDomScheduleLength > 0);
    for (word_t i = 0; i < ksDomScheduleLength; i++) {
        assert(ksDomSchedule[i].domain < CONFIG_NUM_DOMAINS);
        assert(ksDomSchedule[i].length > 0);
    }

    cap_t cap = cap_domain_cap_new();
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapDomain), cap);
}


BOOT_CODE cap_t create_ipcbuf_frame_cap(cap_t root_cnode_cap, cap_t pd_cap, vptr_t vptr)
{
    clearMemory((void *)rootserver.ipc_buf, PAGE_BITS);

    /* create a cap of it and write it into the root CNode */
    cap_t cap = create_mapped_it_frame_cap(pd_cap, rootserver.ipc_buf, vptr, IT_ASID, false, false);
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadIPCBuffer), cap);

    return cap;
}

BOOT_CODE void create_bi_frame_cap(cap_t root_cnode_cap, cap_t pd_cap, vptr_t vptr)
{
    /* create a cap of it and write it into the root CNode */
    cap_t cap = create_mapped_it_frame_cap(pd_cap, rootserver.boot_info, vptr, IT_ASID, false, false);
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapBootInfoFrame), cap);
}

BOOT_CODE word_t calculate_extra_bi_size_bits(word_t extra_size)
{
    if (extra_size == 0) {
        return 0;
    }

    word_t clzl_ret = clzl(ROUND_UP(extra_size, seL4_PageBits));
    word_t msb = seL4_WordBits - 1 - clzl_ret;
    /* If region is bigger than a page, make sure we overallocate rather than underallocate */
    if (extra_size > BIT(msb)) {
        msb++;
    }
    return msb;
}

BOOT_CODE void populate_bi_frame(node_id_t node_id, word_t num_nodes, vptr_t ipcbuf_vptr,
                                 word_t extra_bi_size)
{
    clearMemory((void *) rootserver.boot_info, BI_FRAME_SIZE_BITS);
    if (extra_bi_size) {
        clearMemory((void *) rootserver.extra_bi, calculate_extra_bi_size_bits(extra_bi_size));
    }

    /* initialise bootinfo-related global state */
    ndks_boot.bi_frame = BI_PTR(rootserver.boot_info);
    ndks_boot.slot_pos_cur = seL4_NumInitialCaps;
    BI_PTR(rootserver.boot_info)->nodeID = node_id;
    BI_PTR(rootserver.boot_info)->numNodes = num_nodes;
    BI_PTR(rootserver.boot_info)->numIOPTLevels = 0;
    BI_PTR(rootserver.boot_info)->ipcBuffer = (seL4_IPCBuffer *) ipcbuf_vptr;
    BI_PTR(rootserver.boot_info)->initThreadCNodeSizeBits = CONFIG_ROOT_CNODE_SIZE_BITS;
    BI_PTR(rootserver.boot_info)->initThreadDomain = ksDomSchedule[ksDomScheduleIdx].domain;
    BI_PTR(rootserver.boot_info)->extraLen = extra_bi_size;
}

BOOT_CODE bool_t provide_cap(cap_t root_cnode_cap, cap_t cap)
{
    if (ndks_boot.slot_pos_cur >= ndks_boot.slot_pos_max) {
        printf("Kernel init failed: ran out of cap slots\n");
        return false;
    }
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), ndks_boot.slot_pos_cur), cap);
    ndks_boot.slot_pos_cur++;
    return true;
}

BOOT_CODE create_frames_of_region_ret_t create_frames_of_region(
    cap_t    root_cnode_cap,
    cap_t    pd_cap,
    region_t reg,
    bool_t   do_map,
    sword_t  pv_offset
)
{
    pptr_t     f;
    cap_t      frame_cap;
    seL4_SlotPos slot_pos_before;
    seL4_SlotPos slot_pos_after;

    slot_pos_before = ndks_boot.slot_pos_cur;

    for (f = reg.start; f < reg.end; f += BIT(PAGE_BITS)) {
        if (do_map) {
            frame_cap = create_mapped_it_frame_cap(pd_cap, f, pptr_to_paddr((void *)(f - pv_offset)), IT_ASID, false, true);
        } else {
            frame_cap = create_unmapped_it_frame_cap(f, false);
        }
        if (!provide_cap(root_cnode_cap, frame_cap))
            return (create_frames_of_region_ret_t) {
            S_REG_EMPTY, false
        };
    }

    slot_pos_after = ndks_boot.slot_pos_cur;

    return (create_frames_of_region_ret_t) {
        (seL4_SlotRegion) { slot_pos_before, slot_pos_after }, true
    };
}

BOOT_CODE cap_t create_it_asid_pool(cap_t root_cnode_cap)
{
    cap_t ap_cap = cap_asid_pool_cap_new(IT_ASID >> asidLowBits, rootserver.asid_pool);
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadASIDPool), ap_cap);

    /* create ASID control cap */
    write_slot(
        SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapASIDControl),
        cap_asid_control_cap_new()
    );

    return ap_cap;
}

#ifdef CONFIG_KERNEL_MCS
BOOT_CODE static bool_t configure_sched_context(tcb_t *tcb, sched_context_t *sc_pptr, ticks_t timeslice, word_t core)
{
    tcb->tcbSchedContext = sc_pptr;
    REFILL_NEW(tcb->tcbSchedContext, MIN_REFILLS, timeslice, 0, core);

    tcb->tcbSchedContext->scTcb = tcb;
    return true;
}

BOOT_CODE bool_t init_sched_control(cap_t root_cnode_cap, word_t num_nodes)
{
    seL4_SlotPos slot_pos_before = ndks_boot.slot_pos_cur;

    /* create a sched control cap for each core */
    for (unsigned int i = 0; i < num_nodes; i++) {
        if (!provide_cap(root_cnode_cap, cap_sched_control_cap_new(i))) {
            printf("can't init sched_control for node %u, provide_cap() failed\n", i);
            return false;
        }
    }

    /* update boot info with slot region for sched control caps */
    ndks_boot.bi_frame->schedcontrol = (seL4_SlotRegion) {
        .start = slot_pos_before,
        .end = ndks_boot.slot_pos_cur
    };

    return true;
}
#endif

BOOT_CODE bool_t create_idle_thread(void)
{
    pptr_t pptr;

#ifdef ENABLE_SMP_SUPPORT
    for (unsigned int i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
#endif /* ENABLE_SMP_SUPPORT */
        pptr = (pptr_t) &ksIdleThreadTCB[SMP_TERNARY(i, 0)];
        NODE_STATE_ON_CORE(ksIdleThread, i) = TCB_PTR(pptr + TCB_OFFSET);
        configureIdleThread(NODE_STATE_ON_CORE(ksIdleThread, i));
#ifdef CONFIG_DEBUG_BUILD
        setThreadName(NODE_STATE_ON_CORE(ksIdleThread, i), "idle_thread");
#endif
        SMP_COND_STATEMENT(NODE_STATE_ON_CORE(ksIdleThread, i)->tcbAffinity = i);
#ifdef CONFIG_KERNEL_MCS
        bool_t result = configure_sched_context(NODE_STATE_ON_CORE(ksIdleThread, i), SC_PTR(&ksIdleThreadSC[SMP_TERNARY(i, 0)]),
                                                usToTicks(CONFIG_BOOT_THREAD_TIME_SLICE * US_IN_MS), SMP_TERNARY(i, 0));
        SMP_COND_STATEMENT(NODE_STATE_ON_CORE(ksIdleThread, i)->tcbSchedContext->scCore = i;)
        if (!result) {
            printf("Kernel init failed: Unable to allocate sc for idle thread\n");
            return false;
        }
#endif
#ifdef ENABLE_SMP_SUPPORT
    }
#endif /* ENABLE_SMP_SUPPORT */
    return true;
}

BOOT_CODE tcb_t *create_initial_thread(cap_t root_cnode_cap, cap_t it_pd_cap, vptr_t ui_v_entry, vptr_t bi_frame_vptr,
                                       vptr_t ipcbuf_vptr, cap_t ipcbuf_cap)
{
    tcb_t *tcb = TCB_PTR(rootserver.tcb + TCB_OFFSET);
#ifndef CONFIG_KERNEL_MCS
    tcb->tcbTimeSlice = CONFIG_TIME_SLICE;
#endif

    Arch_initContext(&tcb->tcbArch.tcbContext);

    /* derive a copy of the IPC buffer cap for inserting */
    deriveCap_ret_t dc_ret = deriveCap(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadIPCBuffer), ipcbuf_cap);
    if (dc_ret.status != EXCEPTION_NONE) {
        printf("Failed to derive copy of IPC Buffer\n");
        return NULL;
    }

    /* initialise TCB (corresponds directly to abstract specification) */
    cteInsert(
        root_cnode_cap,
        SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadCNode),
        SLOT_PTR(rootserver.tcb, tcbCTable)
    );
    cteInsert(
        it_pd_cap,
        SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadVSpace),
        SLOT_PTR(rootserver.tcb, tcbVTable)
    );
    cteInsert(
        dc_ret.cap,
        SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadIPCBuffer),
        SLOT_PTR(rootserver.tcb, tcbBuffer)
    );
    tcb->tcbIPCBuffer = ipcbuf_vptr;

    setRegister(tcb, capRegister, bi_frame_vptr);
    setNextPC(tcb, ui_v_entry);

    /* initialise TCB */
#ifdef CONFIG_KERNEL_MCS
    if (!configure_sched_context(tcb, SC_PTR(rootserver.sc), usToTicks(CONFIG_BOOT_THREAD_TIME_SLICE * US_IN_MS), 0)) {
        return NULL;
    }
#endif

    tcb->tcbPriority = seL4_MaxPrio;
    tcb->tcbMCP = seL4_MaxPrio;
    tcb->tcbDomain = ksDomSchedule[ksDomScheduleIdx].domain;
#ifndef CONFIG_KERNEL_MCS
    setupReplyMaster(tcb);
#endif
    setThreadState(tcb, ThreadState_Running);

    ksCurDomain = ksDomSchedule[ksDomScheduleIdx].domain;
#ifdef CONFIG_KERNEL_MCS
    ksDomainTime = usToTicks(ksDomSchedule[ksDomScheduleIdx].length * US_IN_MS);
#else
    ksDomainTime = ksDomSchedule[ksDomScheduleIdx].length;
#endif
    assert(ksCurDomain < CONFIG_NUM_DOMAINS && ksDomainTime > 0);

#ifndef CONFIG_KERNEL_MCS
    SMP_COND_STATEMENT(tcb->tcbAffinity = 0);
#endif

    /* create initial thread's TCB cap */
    cap_t cap = cap_thread_cap_new(TCB_REF(tcb));
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadTCB), cap);

#ifdef CONFIG_KERNEL_MCS
    cap = cap_sched_context_cap_new(SC_REF(tcb->tcbSchedContext), seL4_MinSchedContextBits);
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadSC), cap);
#endif
#ifdef CONFIG_DEBUG_BUILD
    setThreadName(tcb, "rootserver");
#endif

    return tcb;
}

BOOT_CODE void init_core_state(tcb_t *scheduler_action)
{
#ifdef CONFIG_HAVE_FPU
    NODE_STATE(ksActiveFPUState) = NULL;
#endif
#ifdef CONFIG_DEBUG_BUILD
    /* add initial threads to the debug queue */
    NODE_STATE(ksDebugTCBs) = NULL;
    if (scheduler_action != SchedulerAction_ResumeCurrentThread &&
        scheduler_action != SchedulerAction_ChooseNewThread) {
        tcbDebugAppend(scheduler_action);
    }
    tcbDebugAppend(NODE_STATE(ksIdleThread));
#endif
    NODE_STATE(ksSchedulerAction) = scheduler_action;
    NODE_STATE(ksCurThread) = NODE_STATE(ksIdleThread);
#ifdef CONFIG_KERNEL_MCS
    NODE_STATE(ksCurSC) = NODE_STATE(ksCurThread->tcbSchedContext);
    NODE_STATE(ksConsumed) = 0;
    NODE_STATE(ksReprogram) = true;
    NODE_STATE(ksReleaseHead) = NULL;
    NODE_STATE(ksCurTime) = getCurrentTime();
#endif
}

BOOT_CODE static bool_t provide_untyped_cap(
    cap_t      root_cnode_cap,
    bool_t     device_memory,
    pptr_t     pptr,
    word_t     size_bits,
    seL4_SlotPos first_untyped_slot
)
{
    bool_t ret;
    cap_t ut_cap;
    word_t i = ndks_boot.slot_pos_cur - first_untyped_slot;
    if (i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS) {
        ndks_boot.bi_frame->untypedList[i] = (seL4_UntypedDesc) {
            pptr_to_paddr((void *)pptr), size_bits, device_memory, {0}
        };
        ut_cap = cap_untyped_cap_new(MAX_FREE_INDEX(size_bits),
                                     device_memory, size_bits, pptr);
        ret = provide_cap(root_cnode_cap, ut_cap);
    } else {
        printf("Kernel init: Too many untyped regions for boot info\n");
        ret = true;
    }
    return ret;
}

BOOT_CODE static bool_t create_untypeds_for_region(
    cap_t      root_cnode_cap,
    bool_t     device_memory,
    region_t   reg,
    seL4_SlotPos first_untyped_slot
)
{
    word_t align_bits;
    word_t size_bits;

    while (!is_reg_empty(reg)) {
        /* Determine the maximum size of the region */
        size_bits = seL4_WordBits - 1 - clzl(reg.end - reg.start);

        /* Determine the alignment of the region */
        if (reg.start != 0) {
            align_bits = ctzl(reg.start);
        } else {
            align_bits = size_bits;
        }
        /* Reduce size bits to align if needed */
        if (align_bits < size_bits) {
            size_bits = align_bits;
        }
        if (size_bits > seL4_MaxUntypedBits) {
            size_bits = seL4_MaxUntypedBits;
        }

        if (size_bits >= seL4_MinUntypedBits) {
            if (!provide_untyped_cap(root_cnode_cap, device_memory, reg.start, size_bits, first_untyped_slot)) {
                return false;
            }
        }
        reg.start += BIT(size_bits);
    }
    return true;
}

BOOT_CODE bool_t create_untypeds(cap_t root_cnode_cap,
                                 region_t boot_mem_reuse_reg)
{
    seL4_SlotPos first_untyped_slot = ndks_boot.slot_pos_cur;

    paddr_t start = 0;
    for (word_t i = 0; i < ndks_boot.resv_count; i++) {
        if (start < ndks_boot.reserved[i].start) {
            region_t reg = paddr_to_pptr_reg((p_region_t) {
                start, ndks_boot.reserved[i].start
            });
            if (!create_untypeds_for_region(root_cnode_cap, true, reg, first_untyped_slot)) {
                return false;
            }
        }

        start = ndks_boot.reserved[i].end;
    }

    if (start < CONFIG_PADDR_USER_DEVICE_TOP) {
        region_t reg = paddr_to_pptr_reg((p_region_t) {
            start, CONFIG_PADDR_USER_DEVICE_TOP
        });
        /*
         * The auto-generated bitfield code will get upset if the
         * end pptr is larger than the maximum pointer size for this architecture.
         */
        if (reg.end > PPTR_TOP) {
            reg.end = PPTR_TOP;
        }
        if (!create_untypeds_for_region(root_cnode_cap, true, reg, first_untyped_slot)) {
            return false;
        }
    }

    /* if boot_mem_reuse_reg is not empty, we can create UT objs from boot code/data frames */
    if (!create_untypeds_for_region(root_cnode_cap, false, boot_mem_reuse_reg, first_untyped_slot)) {
        return false;
    }

    /* convert remaining freemem into UT objects and provide the caps */
    for (word_t i = 0; i < MAX_NUM_FREEMEM_REG; i++) {
        region_t reg = ndks_boot.freemem[i];
        ndks_boot.freemem[i] = REG_EMPTY;
        if (!create_untypeds_for_region(root_cnode_cap, false, reg, first_untyped_slot)) {
            return false;
        }
    }

    ndks_boot.bi_frame->untyped = (seL4_SlotRegion) {
        .start = first_untyped_slot,
        .end   = ndks_boot.slot_pos_cur
    };

    return true;
}

BOOT_CODE void bi_finalise(void)
{
    seL4_SlotPos slot_pos_start = ndks_boot.slot_pos_cur;
    seL4_SlotPos slot_pos_end = ndks_boot.slot_pos_max;
    ndks_boot.bi_frame->empty = (seL4_SlotRegion) {
        slot_pos_start, slot_pos_end
    };
}

/**
 * Dynamically initialise the available memory on the platform.
 * A region represents an area of memory.
 */
BOOT_CODE bool_t init_freemem(word_t n_available, const p_region_t *available,
                              v_region_t it_v_reg, word_t extra_bi_size_bits)
{
    print_active = 1;
    
    printf("Kernel memory layout\n");
    printf("  phys KERNEL_ELF_PADDR_BASE = %"SEL4_PRIx_word"\n", (word_t)KERNEL_ELF_PADDR_BASE);
    printf("  phys PADDR_TOP             = %"SEL4_PRIx_word"\n", PADDR_TOP);
    printf("       PPTR_BASE_OFFSET      = %"SEL4_PRIx_word"\n", (word_t)PPTR_BASE_OFFSET);
    printf("  virt USER_TOP              = %"SEL4_PRIx_word"\n", (word_t)USER_TOP);
    printf("  virt KERNEL_ELF_BASE       = %"SEL4_PRIx_word"\n", KERNEL_ELF_BASE);
    printf("  virt KDEV_BASE             = %"SEL4_PRIx_word"\n", KDEV_BASE);
    printf("  virt PPTR_TOP              = %"SEL4_PRIx_word"\n", PPTR_TOP);

    /* The system configuration is broken if no region is available */
    if (0 == n_available) {
        printf("ERROR: no memory regions available\n");
        return false;
    }

    /* Force ordering and exclusivity of available regions */
    printf("available memory  regions: %d\n", (int)n_available);
    for (word_t i = 0; i < n_available; i++) {
        const p_region_t *r = &available[i];

        printf("  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n", r->start, r->end);


        /* Available regions must be sane */
        if (r->start > r->end) {
            printf("ERROR: memory region %"SEL4_PRIu_word" has start > end\n", i);
            return false;
        }

        /* Available regions can't be empty */
        if (r->start == r->end) {
            printf("ERROR: memory region %"SEL4_PRIu_word" empty\n", i);
            return false;
        }

        /* regions must be ordered and must not overlap */
        if ((i > 0) && (r->start < available[i - 1].end)) {
            printf("ERROR: memory region %d in wrong order\n", (int)i);
            return false;
        }
    }

    printf("physical memory regions: %d\n", (int)n_available);
    for (word_t i = 0; i < n_available; i++) {
        const p_region_t* r = &available[i];
        printf("  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
               r->start, r->end - 1 );
        assert(r->start < r->end);
    }
    printf("reserved regions: %d\n", (int)ndks_boot.resv_count);
    for (word_t i = 0; i < ndks_boot.resv_count; i++) {
        p_region_t *r = &ndks_boot.reserved[i];
        printf("  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
               r->start, r->end - 1);
        assert(r->start < r->end);
    }

    for (word_t i = 0; i < MAX_NUM_FREEMEM_REG; i++) {
        ndks_boot.freemem[i] = REG_EMPTY;
    }

    word_t idx_a = 0;
    word_t idx_r = 0;
    p_region_t a_tmp = P_REG_EMPTY;
    while ((idx_a < n_available) && (idx_r < ndks_boot.resv_count)) {

        const p_region_t *a = (0 != a_tmp.end) ? &a_tmp : &available[idx_a];
        const p_region_t *r = &ndks_boot.reserved[idx_r];

        /* There can't be any invalid or empty region in the lists. */
        assert( r->start < r->end );
        assert( a->start < a->end );

        if (r->end <= a->start) {
            /* The reserved region is below the available region - skip it. */
            idx_r++;
        } else if (r->start >= a->end) {
            /* The reserved region is above the available region (or what is
             * left of it) - take the whole thing (or remaining chunk).
             */
            insert_region(*a);
            idx_a++;
            a_tmp = P_REG_EMPTY;
        } else if (r->start <= a->start) {
            /* The reserved region overlaps with the start of the available
             * region.
             */
            idx_r++;
            /* Skip the available region if it is fully within the reserved
             * region, otherwise trim start of the available region. */
            if (r->end >= a->end) {
                idx_a++;
                a_tmp = P_REG_EMPTY;
            }
            else
            {
                a_tmp = (p_region_t) { .start = r->end, .end = a->end };
            }

        } else if (r->start < a->end) {
            /* The reserved region overlaps with the end of the available
             * region. Take the first chunk of the available region and move the
             * start to the end of the reserved region
             */
            insert_region((p_region_t) {
                .start = a->start,
                .end   = r->start
            });
            /* Skip the available region if it is fully within the reserved
             * region, otherwise trim start of the available region. */
            if (a->end <= r->end) {
                idx_a++;
                a_tmp = P_REG_EMPTY;
            } else {
                idx_r++;
                a_tmp = (p_region_t) { .start = r->end, .end = a->end };
            }
        } else {
            /* We should never be here, something is wring with the regions. */
            assert(0);
            return false;
        }
    }

    /* no more reserved regions - add the rest */
    while (idx_a < n_available) {
        if (0 != a_tmp.end) {
            insert_region(a_tmp);
            a_tmp = (p_region_t) { .start = 0, .end = 0 };
        }
        else {
            insert_region(available[idx_a]);
        }
        idx_a++;
    }

    printf("reserved regions: %d\n", (int)ndks_boot.resv_count);
    for (word_t i = 0; i < ndks_boot.resv_count; i++) {
        p_region_t *r = &ndks_boot.reserved[i];
        printf("  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
               r->start, r->end - 1);
        assert(r->start < r->end);
    }

    printf("Kernel rootserver setup\n");

    /* now try to fit the root server objects into a region */
    word_t i = MAX_NUM_FREEMEM_REG - 1;
    if (!is_reg_empty(ndks_boot.freemem[i])) {
        printf("Insufficient MAX_NUM_FREEMEM_REG\n");
        return false;
    }
    /* skip any empty regions */
    for (; is_reg_empty(ndks_boot.freemem[i]) && i >= 0; i--) {
        printf("i %d free\n", (int)i);
    }

    /* try to grab the last available p region to create the root server objects
     * from. If possible, retain any left over memory as an extra p region */
    word_t size = calculate_rootserver_size(it_v_reg, extra_bi_size_bits);
    word_t max = rootserver_max_size_bits(extra_bi_size_bits);
    for (; i >= 0; i--) {
        printf("i %d\n", (int)i);
        word_t next = i + 1;
        pptr_t start = ROUND_DOWN(ndks_boot.freemem[i].end - size, max);
        if (start >= ndks_boot.freemem[i].start) {
            create_rootserver_objects(start, it_v_reg, extra_bi_size_bits);
            if (i < MAX_NUM_FREEMEM_REG) {
                ndks_boot.freemem[next].end = ndks_boot.freemem[i].end;
                ndks_boot.freemem[next].start = start + size;
            }
            ndks_boot.freemem[i].end = start;
            break;
        } else if (i < MAX_NUM_FREEMEM_REG) {
            ndks_boot.freemem[next] = ndks_boot.freemem[i];
        }
    }


    printf("Kernel init_freemem done\n");

    return true;
}
