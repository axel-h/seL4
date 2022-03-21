/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <types.h>

static inline irq_t acme_intr_get_claim(void);
static inline void acme_intr_complete_claim(irq_t irq);
static inline void acme_intr_mask_irq(bool_t disable, irq_t irq);
static inline void acme_intr_irq_set_trigger(irq_t irq, bool_t edge_triggered);
static inline void acme_intr_core(void);
static inline void acme_intr_init_controller(void);
