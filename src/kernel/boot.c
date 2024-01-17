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

typedef struct {
#ifdef CONFIG_PRINTING
    const char *name;
#endif
    pptr_t *pObj;
    unsigned int bits;
    unsigned int n;
} rootserver_object_t;

#ifdef CONFIG_PRINTING
#define ROOTSERVER_OBJECT(_name_, _bits_, _n_) \
    { .name = #_name_, .pObj = &rootserver._name_, .bits = _bits_, .n = _n_ }
#else
#define ROOTSERVER_OBJECT(_name_, _bits_, _n_) \
    { .pObj = &rootserver._name_, .bits = _bits_, .n = _n_ }
#endif


/* (node-local) state accessed only during bootstrapping */
BOOT_BSS ndks_boot_t ndks_boot;

BOOT_BSS rootserver_mem_t rootserver;

/* Returns the physical region of the kernel image boot part, which is the part
 * that is no longer needed once booting is finished. */
extern char ki_boot_end[1];
BOOT_CODE p_region_t get_p_reg_kernel_img_boot(void)
{
    return (p_region_t) {
        .start = kpptr_to_paddr((const void *)KERNEL_ELF_BASE),
        .end   = kpptr_to_paddr(ki_boot_end)
    };
}

/* Returns the physical region of the kernel image. */
BOOT_CODE p_region_t get_p_reg_kernel_img(void)
{
    return (p_region_t) {
        .start = kpptr_to_paddr((const void *)KERNEL_ELF_BASE),
        .end   = kpptr_to_paddr(ki_end)
    };
}

BOOT_CODE static inline bool_t is_reg_empty(const region_t reg)
{
    if (reg.start > reg.end) {
        printf("ERROR: is_reg_empty() for invalid region "
               "start=%"SEL4_PRIx_word" end=%"SEL4_PRIx_word"\n",
               reg.start, reg.end);
        // assert(0);
    }

    return (reg.start == reg.end);
}

BOOT_CODE static void merge_regions(void)
{
    /* Walk through reserved regions and see if any can be merged */
    for (word_t i = 1; i < ndks_boot.resv_count;) {
        if (ndks_boot.reserved[i - 1].end == ndks_boot.reserved[i].start) {
            /* extend earlier region */
            ndks_boot.reserved[i - 1].end = ndks_boot.reserved[i].end;
            /* move everything else down */
            for (word_t j = i + 1; j < ndks_boot.resv_count; j++) {
                ndks_boot.reserved[j - 1] = ndks_boot.reserved[j];
            }

            ndks_boot.resv_count--;
            /* don't increment i in case there are multiple adjacent regions */
        } else {
            i++;
        }
    }
}

BOOT_CODE bool_t reserve_region(p_region_t reg)
{
    word_t i;
    assert(reg.start <= reg.end);
    if (reg.start == reg.end) {
        return true;
    }

    /* keep the regions in order */
    for (i = 0; i < ndks_boot.resv_count; i++) {
        /* Try and merge the region to an existing one, if possible */
        if (ndks_boot.reserved[i].start == reg.end) {
            ndks_boot.reserved[i].start = reg.start;
            merge_regions();
            return true;
        }
        if (ndks_boot.reserved[i].end == reg.start) {
            ndks_boot.reserved[i].end = reg.end;
            merge_regions();
            return true;
        }
        /* Otherwise figure out where it should go. */
        if (ndks_boot.reserved[i].start > reg.end) {
            /* move regions down, making sure there's enough room */
            if (ndks_boot.resv_count + 1 >= MAX_NUM_RESV_REG) {
                printf("Can't mark region 0x%"SEL4_PRIx_word"-0x%"SEL4_PRIx_word
                       " as reserved, try increasing MAX_NUM_RESV_REG (currently %d)\n",
                       reg.start, reg.end, (int)MAX_NUM_RESV_REG);
                return false;
            }
            for (word_t j = ndks_boot.resv_count; j > i; j--) {
                ndks_boot.reserved[j] = ndks_boot.reserved[j - 1];
            }
            /* insert the new region */
            ndks_boot.reserved[i] = reg;
            ndks_boot.resv_count++;
            return true;
        }
    }

    if (i + 1 == MAX_NUM_RESV_REG) {
        printf("Can't mark region 0x%"SEL4_PRIx_word"-0x%"SEL4_PRIx_word
               " as reserved, try increasing MAX_NUM_RESV_REG (currently %d)\n",
               reg.start, reg.end, (int)MAX_NUM_RESV_REG);
        return false;
    }

    ndks_boot.reserved[i] = reg;
    ndks_boot.resv_count++;

    return true;
}

BOOT_CODE static bool_t insert_region(region_t reg)
{
    assert(reg.start <= reg.end);
    if (is_reg_empty(reg)) {
        return true;
    }

    for (word_t i = 0; i < ARRAY_SIZE(ndks_boot.freemem); i++) {
        region_t *free_reg = &ndks_boot.freemem[i];
        if (reg.start > reg.end) {
            printf("ERROR: invalid ndks_boot.freemem[%d] "
                   "start=%"SEL4_PRIx_word" end=%"SEL4_PRIx_word"\n",
                   (int)i, reg.start, reg.end);
            // assert(0);
        }
        if (is_reg_empty(*free_reg)) {
            reserve_region(pptr_to_paddr_reg(reg));
            *free_reg = reg;
            return true;
        }
    }

    /* We don't know if a platform or architecture picked MAX_NUM_FREEMEM_REG
     * arbitrarily or carefully calculated it to be big enough. Running out of
     * slots here is not really fatal, eventually memory allocation may fail
     * if there is not enough free memory. However, allocations should never
     * blindly assume to work, some error handling must always be in place even
     * if the environment has been crafted carefully to support them. Thus, we
     * don't stop the boot process here, but return an error. The caller should
     * decide how bad this is.
     */
    printf("no free memory slot left for [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"],"
           " consider increasing MAX_NUM_FREEMEM_REG (%u)\n",
           reg.start, reg.end, (unsigned int)MAX_NUM_FREEMEM_REG);

    /* For debug builds we consider this a fatal error. Rationale is, that the
     * caller does not check the error code at the moment, but just ignores any
     * failures silently. */
    assert(0);

    return false;
}

/* Find a free memory region to create all root server objects to cover the
 * virtual memory region v_reg and any extra boot info.
 */
BOOT_CODE static bool_t create_rootserver_objects(v_region_t it_v_reg,
                                                  word_t extra_bi_size_bits)
{
    /* The free memory regions are set up. Create the rootserver objects. The
     * order in the array below is aligned with the field order use in
     * rootserver_mem_t, but since we also store the pointers here, that is no
     * strictly required. The allocation order will be determined dynamically
     * based on the bit-alignment needs, form highest to lowest.
     */
    word_t cnt_paging = arch_get_n_paging(it_v_reg);
    rootserver_object_t objects[] = {
        ROOTSERVER_OBJECT(cnode, CONFIG_ROOT_CNODE_SIZE_BITS + seL4_SlotBits, 1),
        ROOTSERVER_OBJECT(vspace, seL4_VSpaceBits, 1),
        ROOTSERVER_OBJECT(asid_pool, seL4_ASIDPoolBits, 1),
        ROOTSERVER_OBJECT(ipc_buf, seL4_PageBits, 1),
        ROOTSERVER_OBJECT(boot_info, seL4_BootInfoFrameBits, 1),
        ROOTSERVER_OBJECT(extra_bi, extra_bi_size_bits, 1),
        ROOTSERVER_OBJECT(tcb, seL4_TCBBits, 1),
#ifdef CONFIG_KERNEL_MCS
        ROOTSERVER_OBJECT(sc, seL4_MinSchedContextBits, 1),
#endif
        /* for all archs, seL4_PageTableBits is the size of all non top-level paging structures */
        ROOTSERVER_OBJECT(paging.start, seL4_PageTableBits, cnt_paging)
    };

    /* Calculate the overall size and the max bit alignment. */
    unsigned int align_bits = 0;
    word_t objs_size = 0;
    for (unsigned int i = 0; i < ARRAY_SIZE(objects); i++) {
        rootserver_object_t *obj = &objects[i];
        objs_size += obj->n * BIT(obj->bits);
        if (obj->bits > align_bits) {
            align_bits = obj->bits;
        }
    }

    /* Find a free memory region for all root server objects. Due to the
     * alignment requirements, it's likely there is free space before and after
     * the objects. Making both spaces available as free memory, requires having
     * one additional unused free memory slot. Not having it is considered fatal
     * for now, as the number of slots is just an arbitrary constant that can be
     * updated easily. Also, one additional slot costs almost nothing in terms
     * of memory usage.
     */
    int idx_free = ARRAY_SIZE(ndks_boot.freemem) - 1;
    if (!is_reg_empty(ndks_boot.freemem[idx_free])) {
        printf("MAX_NUM_FREEMEM_REG (%u) to small\n",
               (unsigned int)MAX_NUM_FREEMEM_REG);
        return false;
    }
    pptr_t objs_start;
    for (; idx_free >= 0; idx_free--) {
        region_t *reg = &ndks_boot.freemem[idx_free];
        if (is_reg_empty(*reg)) {
            /* Skip any empty regions. From the check above we know there must
             * be at least one at the end of the list. We do not really expect
             * to see empty regions in the middle of the array.
             */
            continue;
        }
        /* We have this layout in freemem now:
         *   [a] [b] [c] [empty] [...]
         *            ^
         *            potential region for rootserver objects
         */
        region_t *reg_post = &reg[1];
        if (reg->end - reg->start >= objs_size) {
            objs_start = ROUND_DOWN(reg->end - objs_size, align_bits);
            if (objs_start >= reg->start) {
                /* Found a region for the root server objects. Carve out the
                 * memory for the region of the rootserver objects from the free
                 * memory list and put the regions before and after it in
                 * instead so we get this layout in freemem:
                 *   [a] [b] [pre] [post] [...]
                 *                ^
                 *                carved-out region for rootserver objects
                 */
                assert(is_reg_empty(*reg_post));
                *reg_post = (region_t) { /* fill the empty slot */
                    .start = objs_start + objs_size,
                    .end = reg->end
                };
                reg->end = objs_start; /* shrink the region */
                break;
            }
        }
        /* The current region is too small. Since we loop though the list from
         * the end and we know there is an empty region at the end. Thus, the
         * next higher slot must contain am empty region. Swap the slots and
         * try the next lower region so we get this layout in freemem:
         *   [a] [b] [empty] [c] [...]
         *        ^     ^      ^
         *        |     |      region too small
         *        |     potentially used when region b is split
         *        region to check next
         */
        assert(is_reg_empty(*reg_post));
        *reg_post = *reg;
        *reg = REG_EMPTY;
    }

    if (idx_free < 0) {
        printf("ERROR: no free memory region is big enough for root server "
               "objects, need size/alignment of 2^%u\n", align_bits);
        /* Fatal error, can't create root server objects. */
        return false;
    }

    /* Create pptrs for all root server objects in the memory we have carved
     * out. We could soft the list first instead of iterating multiple times,
     * but as long as there are just a few elements there is no significant
     * speed gain here. So we keep iterating and zero each element once we have
     * allocated the memory until all allocation are done.
     */
    printf("allocating root server objects...\n");
    while (align_bits > 0) {
        unsigned int next_align_bits = 0;
        for (unsigned int i = 0; i < ARRAY_SIZE(objects); i++) {
            rootserver_object_t *obj = &objects[i];
            /* ignore unused objects */
            if (0 == obj->n) {
                continue;
            }

            /* We only look at object that match the current size, smaller ones
             * will be handled in another iteration of the outer loop.
             */
            if (align_bits != obj->bits) {
                assert(align_bits > obj->bits);
                if (next_align_bits < obj->bits) {
                    /* Potential candidate for the next outer loop iteration. */
                    next_align_bits = obj->bits;
                }
                continue;
            }

            /* Allocate the object of the current bit-size. Several objects can
             * have the same bit-size, so we could end up here multiple time per
             * loop iteration.
             */
            word_t size = obj->n * BIT(obj->bits);
            printf("  PA [0x%"SEL4_PRIx_word"..0x%"SEL4_PRIx_word"]"
                   " VA [0x%"SEL4_PRIx_word"..0x%"SEL4_PRIx_word"]"
                   ": %u %s object%s of 2^%u (=0x%"SEL4_PRIx_word"/%"SEL4_PRIu_word") byte\n",
                   pptr_to_paddr((void*)objs_start), pptr_to_paddr((void*)(objs_start + size - 1)),
                   objs_start, objs_start + size - 1,
                   obj->n, obj->name, (1 != obj->n) ? "s" : "", obj->bits,
                   BIT(obj->bits), BIT(obj->bits));

            assert(0 == (objs_start % BIT(obj->bits)));
            memzero((void *)objs_start, size);
            /* Remember area for rootserver.xxx objects */
            assert(obj->pObj);
            *(obj->pObj) = objs_start;
            /* Wipe the object in the helper array. */
            memzero(obj, sizeof(*obj)); /* Setting field n to zero is enough */
            /* Adjust free memory. */
            assert(size <= objs_size);
            objs_start += size;
            objs_size -= size;
        }
        /* We are done with allocations for this bit-size. Continue with the
         * next lower size we have found.
         */
        assert(align_bits > next_align_bits);
        align_bits = next_align_bits;
    }

    /* All allocation are done and thus all reserved memory should have been
     * allocated. if there is some left, that memory is lost and it should be
     * investigated why this happened. We consider this fatal for debug builds
     * only, as the size calculation seem broken. Otherwise it is non-fatal,
     * but we print a warning to highlight this.
     */
    if (0 != objs_size) {
        printf("WARNING: %"SEL4_PRIu_word" bytes of unallocated root server "
               "object memory left\n", objs_size);
        assert(0);
    }

    /* set the paging area end. */
    rootserver.paging.end = rootserver.paging.start + (cnt_paging * BIT(seL4_PageTableBits));

    return true;
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
    cap_t cap = cap_cnode_cap_new(
                    CONFIG_ROOT_CNODE_SIZE_BITS, /* radix */
                    wordBits - CONFIG_ROOT_CNODE_SIZE_BITS, /* guard size */
                    0, /* guard */
                    rootserver.cnode); /* pptr */

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
    cap_t cap = create_mapped_it_frame_cap(pd_cap, rootserver.ipc_buf, vptr, IT_ASID, false);
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapInitThreadIPCBuffer), cap);

    return cap;
}

BOOT_CODE void create_bi_frame_cap(cap_t root_cnode_cap, cap_t pd_cap, vptr_t vptr)
{
    /* create a cap of it and write it into the root CNode */
    cap_t cap = create_mapped_it_frame_cap(pd_cap, rootserver.boot_info, vptr, IT_ASID, false);
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), seL4_CapBootInfoFrame), cap);
}

BOOT_CODE word_t calculate_extra_bi_size_bits(word_t extra_size)
{
    if (extra_size == 0) {
        return 0;
    }

    word_t clzl_ret = clzl(ROUND_UP(extra_size, seL4_PageBits));
    word_t msb = seL4_WordBits - 1 - clzl_ret;
    /* If region is bigger than a page, make sure we overallocate rather than
     * underallocate
     */
    if (extra_size > BIT(msb)) {
        msb++;
    }
    return msb;
}

BOOT_CODE void populate_bi_frame(node_id_t node_id, word_t num_nodes,
                                 vptr_t ipcbuf_vptr, word_t extra_bi_size)
{
    /* clear boot info memory */
    clearMemory((void *)rootserver.boot_info, seL4_BootInfoFrameBits);
    if (extra_bi_size) {
        clearMemory((void *)rootserver.extra_bi,
                    calculate_extra_bi_size_bits(extra_bi_size));
    }

    /* initialise bootinfo-related global state */
    seL4_BootInfo *bi = BI_PTR(rootserver.boot_info);

    bi->nodeID = node_id;
    bi->numNodes = num_nodes;
    bi->numIOPTLevels = 0;
    bi->ipcBuffer = (seL4_IPCBuffer *)ipcbuf_vptr;
    bi->initThreadCNodeSizeBits = CONFIG_ROOT_CNODE_SIZE_BITS;
    bi->initThreadDomain = ksDomSchedule[ksDomScheduleIdx].domain;
    bi->extraLen = extra_bi_size;
    /* Technically, setting S_REG_EMPTY is not necessary, because this sets all
     * fields to zero, which should be the state after clearMemory() anyway.
     */
    bi->empty = S_REG_EMPTY;
    bi->sharedFrames = S_REG_EMPTY;
    bi->userImageFrames = S_REG_EMPTY;
    bi->userImagePaging = S_REG_EMPTY;
    bi->ioSpaceCaps = S_REG_EMPTY;
    bi->extraBIPages = S_REG_EMPTY;
#ifdef CONFIG_KERNEL_MCS
    bi->schedcontrol = S_REG_EMPTY;
#endif
    bi->untyped = S_REG_EMPTY;

    ndks_boot.bi_frame = bi;
    ndks_boot.slot_pos_cur = seL4_NumInitialCaps;
}

BOOT_CODE bool_t provide_cap(cap_t root_cnode_cap, cap_t cap)
{
    if (ndks_boot.slot_pos_cur >= BIT(CONFIG_ROOT_CNODE_SIZE_BITS)) {
        printf("ERROR: can't add another cap, all %"SEL4_PRIu_word
               " (=2^CONFIG_ROOT_CNODE_SIZE_BITS) slots used\n",
               BIT(CONFIG_ROOT_CNODE_SIZE_BITS));
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
            frame_cap = create_mapped_it_frame_cap(pd_cap, f, pptr_to_paddr((void *)(f - pv_offset)), IT_ASID, true);
        } else {
            frame_cap = create_unmapped_it_frame_cap(f);
        }
        if (!provide_cap(root_cnode_cap, frame_cap)) {
            return (create_frames_of_region_ret_t) {
                .region  = S_REG_EMPTY,
                .success = false
            };
        }
    }

    slot_pos_after = ndks_boot.slot_pos_cur;

    return (create_frames_of_region_ret_t) {
        .region = (seL4_SlotRegion) {
            .start = slot_pos_before,
            .end   = slot_pos_after
        },
        .success = true
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
BOOT_CODE static void configure_sched_context(tcb_t *tcb, sched_context_t *sc_pptr, ticks_t timeslice)
{
    tcb->tcbSchedContext = sc_pptr;
    refill_new(tcb->tcbSchedContext, MIN_REFILLS, timeslice, 0);
    tcb->tcbSchedContext->scTcb = tcb;
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

BOOT_CODE void create_idle_thread(void)
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
        configure_sched_context(NODE_STATE_ON_CORE(ksIdleThread, i), SC_PTR(&ksIdleThreadSC[SMP_TERNARY(i, 0)]),
                                usToTicks(CONFIG_BOOT_THREAD_TIME_SLICE * US_IN_MS));
        SMP_COND_STATEMENT(NODE_STATE_ON_CORE(ksIdleThread, i)->tcbSchedContext->scCore = i;)
        NODE_STATE_ON_CORE(ksIdleSC, i) = SC_PTR(&ksIdleThreadSC[SMP_TERNARY(i, 0)]);
#endif
#ifdef ENABLE_SMP_SUPPORT
    }
#endif /* ENABLE_SMP_SUPPORT */
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
    configure_sched_context(tcb, SC_PTR(rootserver.sc), usToTicks(CONFIG_BOOT_THREAD_TIME_SLICE * US_IN_MS));
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

#ifdef ENABLE_SMP_CLOCK_SYNC_TEST_ON_BOOT
BOOT_CODE void clock_sync_test(void)
{
    ticks_t t, t0;
    ticks_t margin = usToTicks(1) + getTimerPrecision();

    assert(getCurrentCPUIndex() != 0);
    t = NODE_STATE_ON_CORE(ksCurTime, 0);
    do {
        /* perform a memory acquire to get new values of ksCurTime */
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        t0 = NODE_STATE_ON_CORE(ksCurTime, 0);
    } while (t0 == t);
    t = getCurrentTime();
    printf("clock_sync_test[%d]: t0 = %"PRIu64", t = %"PRIu64", td = %"PRIi64"\n",
           (int)getCurrentCPUIndex(), t0, t, t - t0);
    assert(t0 <= margin + t && t <= t0 + margin);
}
#endif

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

/**
 * Sanity check if a kernel-virtual pointer is in the kernel window that maps
 * physical memory.
 *
 * This check is necessary, but not sufficient, because it only checks for the
 * pointer interval, not for any potential holes in the memory window.
 *
 * @param pptr the pointer to check
 * @return false if the pointer is definitely not in the kernel window, true
 *         otherwise.
 */
BOOT_CODE static bool_t pptr_in_kernel_window(pptr_t pptr)
{
    return pptr >= PPTR_BASE && pptr < PPTR_TOP;
}

/**
 * Create an untyped cap, store it in a cnode and mark it in boot info.
 *
 * The function can fail if basic sanity checks fail, or if there is no space in
 * boot info or cnode to store the cap.
 *
 * @param root_cnode_cap cap to the cnode to store the untyped cap in
 * @param device_memory true if the cap to create is a device untyped
 * @param pptr the kernel-virtual address of the untyped
 * @param size_bits the size of the untyped in bits
 * @param first_untyped_slot next available slot in the boot info structure
 * @return true on success, false on failure
 */
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

    /* Since we are in boot code, we can do extensive error checking and
       return failure if anything unexpected happens. */

    /* Bounds check for size parameter */
    if (size_bits > seL4_MaxUntypedBits || size_bits < seL4_MinUntypedBits) {
        printf("Kernel init: Invalid untyped size %"SEL4_PRIu_word"\n", size_bits);
        return false;
    }

    /* All cap ptrs must be aligned to object size */
    if (!IS_ALIGNED(pptr, size_bits)) {
        printf("Kernel init: Unaligned untyped pptr %p (alignment %"SEL4_PRIu_word")\n", (void *)pptr, size_bits);
        return false;
    }

    /* All cap ptrs apart from device untypeds must be in the kernel window. */
    if (!device_memory && !pptr_in_kernel_window(pptr)) {
        printf("Kernel init: Non-device untyped pptr %p outside kernel window\n",
               (void *)pptr);
        return false;
    }

    /* Check that the end of the region is also in the kernel window, so we don't
       need to assume that the kernel window is aligned up to potentially
       seL4_MaxUntypedBits. */
    if (!device_memory && !pptr_in_kernel_window(pptr + MASK(size_bits))) {
        printf("Kernel init: End of non-device untyped at %p outside kernel window (size %"SEL4_PRIu_word")\n",
               (void *)pptr, size_bits);
        return false;
    }

    word_t i = ndks_boot.slot_pos_cur - first_untyped_slot;
    if (i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS) {
        ndks_boot.bi_frame->untypedList[i] = (seL4_UntypedDesc) {
            .paddr    = pptr_to_paddr((void *)pptr),
            .sizeBits = size_bits,
            .isDevice = device_memory,
            .padding  = {0}
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

/**
 * Create untyped caps for a region of kernel-virtual memory.
 *
 * Takes care of alignement, size and potentially wrapping memory regions. It is fine to provide a
 * region with end < start if the memory is device memory.
 *
 * If the region start is not aligned to seL4_MinUntypedBits, the part up to the next aligned
 * address will be ignored and is lost, because it is too small to create kernel objects in.
 *
 * @param root_cnode_cap Cap to the CNode to store the untypeds in.
 * @param device_memory  Whether the region is device memory.
 * @param reg Region of kernel-virtual memory. May wrap around.
 * @param first_untyped_slot First available untyped boot info slot.
 * @return true on success, false on failure.
 */
BOOT_CODE static bool_t create_untypeds_for_region(
    cap_t      root_cnode_cap,
    bool_t     device_memory,
    region_t   reg,
    seL4_SlotPos first_untyped_slot
)
{
    if (!is_reg_empty(reg)) {
        /* If LSB of end address is set, we consider this as an inclusive
         * address, otherwise it's considered an exclusive address and we
         * subtract one
         */
        printf("provide caps for PA [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"], "
               "VA [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"] (%s memory)\n",
               pptr_to_paddr((void *)reg.start),
               pptr_to_paddr((void *)(reg.end - (~reg.end & 1))),
               reg.start,
               reg.end - (~reg.end & 1),
               device_memory ? "device" : "untyped");
    }

    /* This code works with regions that wrap (where end < start), because the loop cuts up the
       region into size-aligned chunks, one for each cap. Memory chunks that are size-aligned cannot
       themselves overflow, so they satisfy alignment, size, and overflow conditions. The region
       [0..end) is not necessarily part of the kernel window (depending on the value of PPTR_BASE).
       This is fine for device untypeds. For normal untypeds, the region is assumed to be fully in
       the kernel window. This is not checked here. */
    while (!is_reg_empty(reg)) {

        /* Calculate the bit size of the region. This is also correct for end < start: it will
           return the correct size of the set [start..-1] union [0..end). This will then be too
           large for alignment, so the code further down will reduce the size. */
        unsigned int size_bits = seL4_WordBits - 1 - clzl(reg.end - reg.start);
        /* The size can't exceed the largest possible untyped size. */
        if (size_bits > seL4_MaxUntypedBits) {
            size_bits = seL4_MaxUntypedBits;
        }
        /* The start address 0 satisfies any alignment needs, otherwise ensure
         * the region's bit size does not exceed the alignment of the region.
         */
        if (0 != reg.start) {
            unsigned int align_bits = ctzl(reg.start);
            if (size_bits > align_bits) {
                size_bits = align_bits;
            }
        }
        /* Provide an untyped capability for the region only if it is large
         * enough to be retyped into objects later. Otherwise the region can't
         * be used anyway.
         */
        if (size_bits >= seL4_MinUntypedBits) {
            if (!provide_untyped_cap(root_cnode_cap, device_memory, reg.start, size_bits, first_untyped_slot)) {
                return false;
            }
        }
        reg.start += BIT(size_bits);
    }
    return true;
}

BOOT_CODE bool_t create_untypeds(cap_t root_cnode_cap)
{
    seL4_SlotPos first_untyped_slot = ndks_boot.slot_pos_cur;

    printf("create device untypeds\n");
    paddr_t start = 0;
    for (word_t i = 0; i < ndks_boot.resv_count; i++) {
        if (start < ndks_boot.reserved[i].start) {
            region_t reg = paddr_to_pptr_reg((p_region_t) {
                start, ndks_boot.reserved[i].start
            });
            if (!create_untypeds_for_region(root_cnode_cap, true, reg, first_untyped_slot)) {
                printf("ERROR: creation of untypeds for device region #%u at"
                       " [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"] failed\n",
                       (unsigned int)i, reg.start, reg.end);
                return false;
            }
        }

        start = ndks_boot.reserved[i].end;
    }

    if (start < CONFIG_PHYS_ADDR_TOP) {
        region_t reg = paddr_to_pptr_reg((p_region_t) {
            .start = start,
            .end = CONFIG_PHYS_ADDR_TOP
        });

        if (!create_untypeds_for_region(root_cnode_cap, true, reg, first_untyped_slot)) {
            printf("ERROR: creation of untypeds for top device region"
                   " [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"] failed\n",
                   reg.start, reg.end);
            return false;
        }
    }

    /* There is a part of the kernel (code/data) that is only needed for the
     * boot process. We can create UT objects for these frames, so the memory
     * can be reused.
     */
    printf("create memory untypeds\n");
    region_t boot_mem_reuse_reg = paddr_to_pptr_reg(get_p_reg_kernel_img_boot());
    if (!create_untypeds_for_region(root_cnode_cap, false, boot_mem_reuse_reg, first_untyped_slot)) {
        printf("ERROR: creation of untypeds for recycled boot memory"
               " [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"] failed\n",
               pptr_to_paddr((void *)boot_mem_reuse_reg.start),
               pptr_to_paddr((void *)boot_mem_reuse_reg.end));
        return false;
    }

    /* convert remaining freemem into UT objects and provide the caps */
    for (word_t i = 0; i < ARRAY_SIZE(ndks_boot.freemem); i++) {
        region_t reg = ndks_boot.freemem[i];
        ndks_boot.freemem[i] = REG_EMPTY;
        if (!create_untypeds_for_region(root_cnode_cap, false, reg, first_untyped_slot)) {
            printf("ERROR: creation of untypeds for free memory region #%u at"
                   " [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"] failed\n",
                   (unsigned int)i, pptr_to_paddr((void *)reg.start),
                   pptr_to_paddr((void *)reg.end));
            return false;
        }
    }

    printf("untyped slots usage %d/%d\n",
           (int)(ndks_boot.bi_frame->untyped.end - ndks_boot.bi_frame->untyped.start),
           (int)CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS);

    ndks_boot.bi_frame->untyped = (seL4_SlotRegion) {
        .start = first_untyped_slot,
        .end   = ndks_boot.slot_pos_cur
    };

    printf("create untypeds done\n");

    return true;
}

BOOT_CODE void bi_finalise(void)
{
    ndks_boot.bi_frame->empty = (seL4_SlotRegion) {
        .start = ndks_boot.slot_pos_cur,
        .end   = BIT(CONFIG_ROOT_CNODE_SIZE_BITS)
    };
}

BOOT_CODE static inline pptr_t ceiling_kernel_window(pptr_t p)
{
    /* Adjust address if it exceeds the kernel window
     * Note that we compare physical address in case of overflow.
     */
    if (pptr_to_paddr((void *)p) > PADDR_TOP) {
        p = PPTR_TOP;
    }
    return p;
}

BOOT_CODE static bool_t check_available_memory(word_t n_available,
                                               const p_region_t *available)
{
    /* The system configuration is broken if no region is available. */
    if (0 == n_available) {
        printf("ERROR: no memory regions available\n");
        return false;
    }

    printf("available phys memory regions: %"SEL4_PRIu_word"\n", n_available);
    /* Force ordering and exclusivity of available regions. */
    for (word_t i = 0; i < n_available; i++) {
        const p_region_t *r = &available[i];
        printf("  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n", r->start, r->end);

        /* Available regions must be sane */
        if (r->start > r->end) {
            printf("ERROR: memory region %"SEL4_PRIu_word" has start > end\n", i);
            return false;
        }

        /* Available regions can't be empty. */
        if (r->start == r->end) {
            printf("ERROR: memory region %"SEL4_PRIu_word" empty\n", i);
            return false;
        }

        /* Regions must be ordered and must not overlap. Regions are [start..end),
           so the == case is fine. Directly adjacent regions are allowed. */
        if ((i > 0) && (r->start < available[i - 1].end)) {
            printf("ERROR: memory region %d in wrong order\n", (int)i);
            return false;
        }
    }

    return true;
}


BOOT_CODE static bool_t check_reserved_memory(word_t n_reserved,
                                              const region_t *reserved)
{
    printf("reserved address space regions: %"SEL4_PRIu_word"\n",
           n_reserved);
    /* Force ordering and exclusivity of reserved regions. */
    for (word_t i = 0; i < n_reserved; i++) {
        const region_t *r = &reserved[i];
        printf("  [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
               pptr_to_paddr((void *)r->start), pptr_to_paddr((void *)r->end));

        /* Reserved regions must be sane, the size is allowed to be zero. */
        if (r->start > r->end) {
            printf("ERROR: reserved region %"SEL4_PRIu_word" has start > end\n", i);
            return false;
        }

        /* Regions must be ordered and must not overlap. Regions are [start..end),
           so the == case is fine. Directly adjacent regions are allowed. */
        if ((i > 0) && (r->start < reserved[i - 1].end)) {
            printf("ERROR: reserved region %"SEL4_PRIu_word" in wrong order\n", i);
            return false;
        }
    }

    return true;
}

/* we can't declare arrays on the stack, so this is space for
 * the function below to use. */
BOOT_BSS static region_t avail_reg[MAX_NUM_FREEMEM_REG];
/**
 * Dynamically initialise the available memory on the platform.
 * A region represents an area of memory.
 */
BOOT_CODE bool_t init_freemem(word_t n_available, const p_region_t *available,
                              word_t n_reserved, const region_t *reserved,
                              v_region_t it_v_reg, word_t extra_bi_size_bits)
{
    printf("kernel window: PA: [%p..%p] VA: [%p..%p]\n",
           (void *)PPTR_BASE, (void *)PPTR_TOP,
           (void *)PADDR_BASE, (void *)PADDR_TOP);
    /* Sanity check: The defines about the kernel window must be consistent. If
     * this fails the architecture or platform definitions are broken. */
    assert((word_t)paddr_to_pptr(PADDR_BASE) == PPTR_BASE);
    assert((word_t)paddr_to_pptr(PADDR_TOP) == PPTR_TOP);
    assert(pptr_to_paddr((void *)PPTR_BASE) == PADDR_BASE);
    assert(pptr_to_paddr((void *)PPTR_TOP) == PADDR_TOP);

    if (!check_available_memory(n_available, available)) {
        return false;
    }

    if (!check_reserved_memory(n_reserved, reserved)) {
        return false;
    }

    for (word_t i = 0; i < ARRAY_SIZE(ndks_boot.freemem); i++) {
        ndks_boot.freemem[i] = REG_EMPTY;
    }

    /* convert the available regions to pptrs */
    for (word_t i = 0; i < n_available; i++) {
        avail_reg[i] = paddr_to_pptr_reg(available[i]);
        avail_reg[i].end = ceiling_kernel_window(avail_reg[i].end);
        avail_reg[i].start = ceiling_kernel_window(avail_reg[i].start);
    }

    word_t a = 0;
    word_t r = 0;
    /* Now iterate through the available regions, removing any reserved regions. */
    while (a < n_available && r < n_reserved) {
        if (reserved[r].start == reserved[r].end) {
            /* reserved region is empty - skip it */
            r++;
        } else if (avail_reg[a].start >= avail_reg[a].end) {
            /* skip the entire region - it's empty now after trimming */
            a++;
        } else if (reserved[r].end <= avail_reg[a].start) {
            /* the reserved region is below the available region - skip it */
            reserve_region(pptr_to_paddr_reg(reserved[r]));
            r++;
        } else if (reserved[r].start >= avail_reg[a].end) {
            /* the reserved region is above the available region - take the whole thing */
            insert_region(avail_reg[a]);
            a++;
        } else {
            /* the reserved region overlaps with the available region */
            if (reserved[r].start <= avail_reg[a].start) {
                /* the region overlaps with the start of the available region.
                 * trim start of the available region */
                avail_reg[a].start = MIN(avail_reg[a].end, reserved[r].end);
                reserve_region(pptr_to_paddr_reg(reserved[r]));
                r++;
            } else {
                assert(reserved[r].start < avail_reg[a].end);
                /* take the first chunk of the available region and move
                 * the start to the end of the reserved region */
                region_t m = avail_reg[a];
                m.end = reserved[r].start;
                insert_region(m);
                if (avail_reg[a].end > reserved[r].end) {
                    avail_reg[a].start = reserved[r].end;
                    reserve_region(pptr_to_paddr_reg(reserved[r]));
                    r++;
                } else {
                    a++;
                }
            }
        }
    }

    for (; r < n_reserved; r++) {
        if (reserved[r].start < reserved[r].end) {
            reserve_region(pptr_to_paddr_reg(reserved[r]));
        }
    }

    /* no more reserved regions - add the rest */
    for (; a < n_available; a++) {
        if (avail_reg[a].start < avail_reg[a].end) {
            insert_region(avail_reg[a]);
        }
    }

    /* The free memory regions are set up, try to fit the root server objects
     * into one of the regions.
     */
    if (!create_rootserver_objects(it_v_reg, extra_bi_size_bits)) {
        printf("ERROR: could not create root server objects\n");
        return false;
    }

    printf("free memory regions:\n");
#ifdef CONFIG_PRINTING
    for (unsigned int j = 0; j < ARRAY_SIZE(ndks_boot.freemem); j++) {
        const region_t *reg = &ndks_boot.freemem[j];
        if (!is_reg_empty(*reg)) {
            printf("  PA [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"], "
                   "VA [%"SEL4_PRIx_word"..%"SEL4_PRIx_word"]\n",
                   pptr_to_paddr((void *)reg->start),
                   pptr_to_paddr((void *)(reg->end - 1)),
                   reg->start,
                   reg->end - 1);
        }
    }
#endif /* CONFIG_PRINTING */

    return true;
}
