/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <mode/object/structures.h>

#define tcbArchCNodeEntries tcbCNodeEntries

static inline bool_t CONST Arch_isCapRevocable(cap_t derivedCap, cap_t srcCap)
{
    switch (cap_get_capType(derivedCap)) {
#ifdef CONFIG_ALLOW_SMC_CALLS
    case cap_smc_cap:
        return (cap_smc_cap_get_capSMCBadge(derivedCap) !=
                cap_smc_cap_get_capSMCBadge(srcCap));
#endif
    default:
        return false;
    }
}
