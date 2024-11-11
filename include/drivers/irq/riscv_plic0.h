/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * SiFive U54/U74 PLIC handling (HiFive Unleashed/Unmatched, Polarfire,
 * QEMU RISC-V virt, Star64)
 */

#pragma once

/* This is a check that prevents using this driver blindly. Extend the list if
 * this driver is confirmed to be working on other platforms. */
#if !defined(CONFIG_PLAT_HIFIVE) && \
    !defined(CONFIG_PLAT_POLARFIRE) && \
    !defined(CONFIG_PLAT_QEMU_RISCV_VIRT) && \
    !defined(CONFIG_PLAT_ROCKETCHIP_ZCU102) && \
    !defined(CONFIG_PLAT_STAR64) && \
    !defined(CONFIG_PLAT_ARIANE)
#error "Check if this platform supports a PLIC."
#endif

/* tell the kernel we have the set trigger feature */
#define HAVE_SET_TRIGGER 1

#include <plat/machine/devices_gen.h>
#include <arch/model/smp.h>
#include <arch/machine/plic.h>

/* The memory map is based on the PLIC section in
 * https://static.dev.sifive.com/U54-MC-RVCoreIP.pdf
 */

#define PLIC_HART_ID (CONFIG_FIRST_HART_ID)

#define PLIC_PRIO               0x0
#define PLIC_PRIO_PER_ID        0x4

#define PLIC_PENDING            0x1000
#define PLIC_EN                 0x2000
#define PLIC_EN_PER_HART        0x100
#define PLIC_EN_PER_CONTEXT     0x80


#define PLIC_THRES              0x200000
#define PLIC_SVC_CONTEXT        1
#define PLIC_THRES_PER_HART     0x2000
#define PLIC_THRES_PER_CONTEXT  0x1000
#define PLIC_THRES_CLAIM        0x4

#define PLIC_NUM_INTERRUPTS PLIC_MAX_IRQ

#if defined(CONFIG_PLAT_HIFIVE) || \
    defined(CONFIG_PLAT_POLARFIRE) || \
    defined(CONFIG_PLAT_STAR64)

/* SiFive U54-MC and U74-MC have 5 cores, and the first core does not have
 * supervisor mode. Therefore, we need to compensate for the addresses.
 */
#define PLAT_PLIC_THRES_ADJUST(x) ((x) - PLIC_THRES_PER_CONTEXT)
#define PLAT_PLIC_EN_ADJUST(x)    ((x) - PLIC_EN_PER_CONTEXT)

#else

#define PLAT_PLIC_THRES_ADJUST(x)   (x)
#define PLAT_PLIC_EN_ADJUST(x)      (x)

#endif



static inline uint32_t plic_read_u32(word_t offset)
{
    return *((volatile uint32_t *)(PLIC_PPTR + offset));
}

static inline void plic_write_u32(uint32_t val, word_t offset)
{
    *((volatile uint32_t *)(PLIC_PPTR + offset)) = val;
}

static inline word_t plic_enable_offset(word_t hart_id, word_t context_id)
{
    word_t addr = PLAT_PLIC_EN_ADJUST(PLIC_EN + hart_id * PLIC_EN_PER_HART + context_id * PLIC_EN_PER_CONTEXT);
    return addr;
}


static inline word_t plic_thres_offset(word_t hart_id, word_t context_id)
{
    word_t addr = PLAT_PLIC_THRES_ADJUST(PLIC_THRES + hart_id * PLIC_THRES_PER_HART + context_id * PLIC_THRES_PER_CONTEXT);
    return addr;
}

static inline word_t plic_claim_offset(word_t hart_id, word_t context_id)
{
    word_t addr = plic_thres_offset(hart_id, context_id) + PLIC_THRES_CLAIM;
    return addr;
}

static inline bool_t plic_pending_interrupt(word_t interrupt)
{
    word_t offset = PLIC_PENDING + (interrupt / 32) * 4;
    word_t bit = interrupt % 32;
    if (plic_read_u32(offset) & BIT(bit)) {
        return true;
    } else {
        return false;
    }
}

/* The PLIC has separate register sets for each hart and the hart's context.
 * This returns the hart ID used by the PLIC for the hart this code is currently
 * executing on.
 */
static inline word_t plic_get_current_hart_id(void)
{
    return SMP_TERNARY(
               cpuIndexToID(getCurrentCPUIndex()),
               CONFIG_FIRST_HART_ID);
}

static inline irq_t plic_get_claim(void)
{
    /* Read the claim register for our HART interrupt context */
    word_t hart_id = plic_get_current_hart_id();
    return plic_read_u32(plic_claim_offset(hart_id, PLIC_SVC_CONTEXT));
}

static inline void plic_complete_claim(irq_t irq)
{
    /* Complete the IRQ claim by writing back to the claim register. */
    word_t hart_id = plic_get_current_hart_id();
    plic_write_u32(irq, plic_claim_offset(hart_id, PLIC_SVC_CONTEXT));
}

static inline void plic_mask_irq(bool_t disable, irq_t irq)
{
    word_t hart_id = plic_get_current_hart_id();
    word_t offset = plic_enable_offset(hart_id, PLIC_SVC_CONTEXT) + (irq / 32) * 4;
    uint32_t bit = irq % 32;
    uint32_t val = plic_read_u32(offset);
    if (disable) {
        val &= ~BIT(bit);
    } else {
        val |= BIT(bit);
    }
    plic_write_u32(val, offset);
}

static inline void plic_init_hart(void)
{

    word_t hart_id = plic_get_current_hart_id();

    for (int i = 1; i <= PLIC_NUM_INTERRUPTS; i++) {
        /* Disable interrupts */
        plic_mask_irq(true, i);
    }

    /* Set threshold to zero */
    plic_write_u32(0, plic_thres_offset(hart_id, PLIC_SVC_CONTEXT));
}

static inline void plic_init_controller(void)
{
    word_t offset = plic_claim_offset(PLIC_HART_ID, PLIC_SVC_CONTEXT);
    for (int i = 1; i <= PLIC_NUM_INTERRUPTS; i++) {
        /* Clear all pending bits */
        if (plic_pending_interrupt(i)) {
            plic_read_u32(offset);
            plic_write_u32(i, offset);
        }
    }

    /* Set the priorities of all interrupts to 1 */
    for (int i = 1; i <= PLIC_MAX_IRQ + 1; i++) {
        plic_write_u32(2, PLIC_PRIO + PLIC_PRIO_PER_ID * i);
    }

}


/*
 * Provide a dummy definition of set trigger as the Hifive platform currently
 * has all global interrupts positive-level triggered.
 */
static inline void plic_irq_set_trigger(irq_t irq, bool_t edge_triggered)
{
}
