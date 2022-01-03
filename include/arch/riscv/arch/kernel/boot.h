/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <types.h>

/* This is called from assembly code and thus there are no specific types in
 * the signature.
 */
void init_kernel(
    word_t ui_p_reg_start,
    word_t ui_p_reg_end,
    word_t pv_offset,
    word_t v_entry,
    word_t dtb_addr_p,
    word_t dtb_size
#ifdef ENABLE_SMP_SUPPORT
    ,
    word_t hart_id,
    word_t core_id
#endif
);


