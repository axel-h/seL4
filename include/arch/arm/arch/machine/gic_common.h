/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <stdint.h>
#include <machine/interrupt.h>

/* Shift positions for GICD_SGIR register */
#define GICD_SGIR_SGIINTID_SHIFT          0
#define GICD_SGIR_CPUTARGETLIST_SHIFT     16
#define GICD_SGIR_TARGETLISTFILTER_SHIFT  24

enum {
    /*
     * The ARM GIC reserves interrupt 0-15 for SGIs (Software Generated
     * Interrupt) and 16-31 for PPIs (Private Peripheral Interrupt). Both SGIs
     * and PPIs are banked per core. Interrupt 32 and above are SPIs (Shared
     * Peripheral Interrupt), which are global.
     */
    gic_irq_sgi_start               = 0,
#ifdef ENABLE_SMP_SUPPORT
    /* Use the first two SGIs for the IPI implementation */
    gic_irq_remote_call_ipi         = 0,
    gic_irq_reschedule_ipi          = 1,
#endif /* ENABLE_SMP_SUPPORT */
    gic_irq_ppi_start               = 16,
    gic_irq_spi_start               = 32,
    gic_irq_special_start           = 1020,
    gic_irq_none                    = 1023,
    gic_irq_invalid                 = (word_t)(-1)
} gic_interrupts_t;

#define SGI_START         ((word_t)gic_irq_sgi_start)
#define PPI_START         ((word_t)gic_irq_ppi_start)
#define SPI_START         ((word_t)gic_irq_spi_start)
#define SPECIAL_IRQ_START ((word_t)gic_irq_special_start)
#define IRQ_NONE          ((word_t)gic_irq_none)

/* ToDo: There is a CONFIG_NUM_PPI also, that is not sync'd with this define */
#define NUM_PPI           SPI_START

#define HW_IRQ_IS_SGI(irq) ((irq) < PPI_START)
#define HW_IRQ_IS_PPI(irq) ((irq) < NUM_PPI)

#ifdef ENABLE_SMP_SUPPORT
/* In this case irq_t is a struct with an hw irq field and target core field.
 * The following macros convert between (target_core, hw_irq) <-> irq_t <-> cnode index.
 * IRQ_IS_PPI returns true if hw_irq < 32 which is a property of the GIC.
 * The layout of IRQs into the CNode are all of PPI's for each core first, followed
 * by the global interrupts.  Examples:
 *   core: 0, irq: 12 -> index 12.
 *   core: 2, irq: 16 -> (2 * 32) + 16
 *   core: 1, irq: 33, (4 total cores) -> (4 * 32) + (33-32).
 */
#define IRQ_IS_PPI(_irq) (HW_IRQ_IS_PPI(_irq.irq))

#define IRQT_TO_IDX(_irq) (HW_IRQ_IS_PPI(_irq.irq) ? \
                                 (_irq.target_core) * NUM_PPI + (_irq.irq) : \
                                 (CONFIG_MAX_NUM_NODES - 1) * NUM_PPI + (_irq.irq))

#define IDX_TO_IRQT(idx) (((idx) < NUM_PPI*CONFIG_MAX_NUM_NODES) ? \
                        CORE_IRQ_TO_IRQT((idx) / NUM_PPI, (idx) - ((idx)/NUM_PPI)*NUM_PPI): \
                        CORE_IRQ_TO_IRQT(0, (idx) - (CONFIG_MAX_NUM_NODES-1)*NUM_PPI))

#else /* not ENABLE_SMP_SUPPORT */

#define IRQ_IS_PPI(irq) HW_IRQ_IS_PPI(irq)
#endif /* [not] ENABLE_SMP_SUPPORT */

/* Setters/getters helpers for hardware irqs */
#define IRQ_REG(IRQ) ((IRQ) >> 5u)
#define IRQ_BIT(IRQ) ((IRQ) & 0x1f)
#define IS_IRQ_VALID(X) (((X) & IRQ_MASK) < SPECIAL_IRQ_START)

/*
 * The only sane way to get an GIC IRQ number that can be properly
 * ACKED later is through the int_ack register. Unfortunately, reading
 * this register changes the interrupt state to pending so future
 * reads will not return the same value For this reason, we have a
 * global variable to store the IRQ number.
 */
extern word_t active_irq[CONFIG_MAX_NUM_NODES];

static inline void handleSpuriousIRQ(void)
{
}

void initIRQController(void);


