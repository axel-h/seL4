/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <types.h>
#include <object.h>
#include <api/failures.h>
#include <kernel/thread.h>
#include <kernel/cspace.h>
#include <model/statedata.h>
#include <arch/machine.h>

lookupCap_ret_t lookupCap(tcb_t *thread, cptr_t cPtr)
{
    lookupSlot_raw_ret_t lu_ret = lookupSlot(thread, cPtr);
    return (lookupCap_ret_t) {
        .status = lu_ret.status,
        .cap    = likely(lu_ret.status == EXCEPTION_NONE) ? lu_ret.slot->cap : cap_null_cap_new(),
    };
}

lookupCapAndSlot_ret_t lookupCapAndSlot(tcb_t *thread, cptr_t cPtr)
{
    lookupSlot_raw_ret_t lu_ret = lookupSlot(thread, cPtr);
    return (lookupCapAndSlot_ret_t) {
                .status = lu_ret.status,
                .slot   = lu_ret.slot, /* it's NULL on error */
                .cap    = likely(lu_ret.status == EXCEPTION_NONE) ? lu_ret.slot->cap : cap_null_cap_new(),
            };
}

lookupSlot_raw_ret_t lookupSlot(tcb_t *thread, cptr_t capptr)
{
    cap_t threadRoot = TCB_PTR_CTE_PTR(thread, tcbCTable)->cap;
    resolveAddressBits_ret_t res_ret = resolveAddressBits(threadRoot, capptr, wordBits);
    return (lookupSlot_raw_ret_t) {
        .status = res_ret.status,
        .slot   = res_ret.slot, /* it's NULL on error */
    };
}

lookupSlot_ret_t lookupSlotForCNodeOp(bool_t isSource, cap_t root, cptr_t capptr,
                                      word_t depth)
{
    if (unlikely(cap_get_capType(root) != cap_cnode_cap)) {
        current_syscall_error.type = seL4_FailedLookup;
        current_syscall_error.failedLookupWasSource = isSource;
        current_lookup_fault = lookup_fault_invalid_root_new();
        return (lookupSlot_ret_t) {
            .status = EXCEPTION_SYSCALL_ERROR,
            .slot   = NULL,

        };
    }

    if (unlikely(depth < 1 || depth > wordBits)) {
        current_syscall_error.type = seL4_RangeError;
        current_syscall_error.rangeErrorMin = 1;
        current_syscall_error.rangeErrorMax = wordBits;
        return (lookupSlot_ret_t) {
            .status = EXCEPTION_SYSCALL_ERROR,
            .slot   = NULL,
        };
    }

    resolveAddressBits_ret_t res_ret = resolveAddressBits(root, capptr, depth);
    if (unlikely(res_ret.status != EXCEPTION_NONE)) {
        current_syscall_error.type = seL4_FailedLookup;
        current_syscall_error.failedLookupWasSource = isSource;
        /* current_lookup_fault will have been set by resolveAddressBits */
        return (lookupSlot_ret_t) {
            .status = EXCEPTION_SYSCALL_ERROR,
            .slot   = NULL,
        };
    }

    if (unlikely(res_ret.bitsRemaining != 0)) {
        current_syscall_error.type = seL4_FailedLookup;
        current_syscall_error.failedLookupWasSource = isSource;
        current_lookup_fault =
            lookup_fault_depth_mismatch_new(0, res_ret.bitsRemaining);
        return (lookupSlot_ret_t) {
            .status = EXCEPTION_SYSCALL_ERROR,
            .slot   = NULL,
        };
    }

    return (lookupSlot_ret_t) {
        .status = EXCEPTION_NONE,
        .slot   = res_ret.slot,
    };
}

lookupSlot_ret_t lookupSourceSlot(cap_t root, cptr_t capptr, word_t depth)
{
    return lookupSlotForCNodeOp(true, root, capptr, depth);
}

lookupSlot_ret_t lookupTargetSlot(cap_t root, cptr_t capptr, word_t depth)
{
    return lookupSlotForCNodeOp(false, root, capptr, depth);
}

lookupSlot_ret_t lookupPivotSlot(cap_t root, cptr_t capptr, word_t depth)
{
    return lookupSlotForCNodeOp(true, root, capptr, depth);
}

resolveAddressBits_ret_t resolveAddressBits(cap_t nodeCap, cptr_t capptr, word_t n_bits)
{
    assert(n_bits <= BIT(radixBits));

    const resolveAddressBits_ret_t resolve_error = {
        .status        = EXCEPTION_LOOKUP_FAULT,
        .slot          = NULL,
        .bitsRemaining = n_bits,
    };

    if (unlikely(cap_get_capType(nodeCap) != cap_cnode_cap)) {
        current_lookup_fault = lookup_fault_invalid_root_new();
        return resolve_error;
    }


    while (1) {
        const word_t radixBits = cap_cnode_cap_get_capCNodeRadix(nodeCap);
        const word_t guardBits = cap_cnode_cap_get_capCNodeGuardSize(nodeCap);
        const word_t levelBits = radixBits + guardBits;

        /* Haskell error: "All CNodes must resolve bits" */
        assert(levelBits != 0);

        const word_t capGuard = cap_cnode_cap_get_capCNodeGuard(nodeCap);

        /* The MASK(wordRadix) here is to avoid the case where
         * n_bits = wordBits (=2^wordRadix) and guardBits = 0, as it violates
         * the C spec to shift right by more than wordBits-1.
         */
        const word_t guard = (0 == guardBits) ? 0 : (capptr >> (n_bits - guardBits)) & MASK(guardBits);
        if (unlikely(guardBits > n_bits || guard != capGuard)) {
            current_lookup_fault =
                lookup_fault_guard_mismatch_new(capGuard, n_bits, guardBits);
            return resolve_error;
        }

        if (unlikely(levelBits > n_bits)) {
            current_lookup_fault =
                lookup_fault_depth_mismatch_new(levelBits, n_bits);
            return resolve_error;
        }

        const word_t offset = (capptr >> (n_bits - levelBits)) & MASK(radixBits);
        cte_t * const slot = CTE_PTR(cap_cnode_cap_get_capCNodePtr(nodeCap)) + offset;

        if (likely(n_bits == levelBits)) {
            return (resolveAddressBits_ret_t) {
                .status        = EXCEPTION_NONE,
                .slot          = slot,
                .bitsRemaining = 0,
            };
        }

        /** GHOSTUPD: "(\<acute>levelBits > 0, id)" */

        n_bits -= levelBits;
        nodeCap = slot->cap;

        if (unlikely(cap_get_capType(nodeCap) != cap_cnode_cap)) {
            return (resolveAddressBits_ret_t) {
                .status        = EXCEPTION_NONE,
                .slot          = slot,
                .bitsRemaining = n_bits,
            };
        }
    }

    UNREACHABLE();
}
