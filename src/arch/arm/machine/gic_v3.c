/*
 * Copyright 2019, DornerWorks
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <types.h>
#include <arch/machine/gic_v3.h>

#define IRQ_SET_ALL 0xffffffff

#define RDIST_BANK_SZ 0x00010000
/* One GICR region and one GICR_SGI region */
#define GICR_PER_CORE_SIZE  (0x20000)
/* Assume 8 cores */
#define GICR_SIZE           (0x100000)

#define GIC_DEADLINE_MS 2
#define GIC_REG_WIDTH   32

#ifdef CONFIG_ARCH_AARCH64
#define ICC_SGI1R_EL1 "S3_0_C12_C11_5"
#else
#define ICC_SGI1R_EL1 "p15, 0, %Q0, %R0, c12"
#endif

#define ICC_SGI1R_INTID_SHIFT          (24)
#define ICC_SGI1R_AFF1_SHIFT           (16)
#define ICC_SGI1R_IRM_BIT              (40)
#define ICC_SGI1R_CPUTARGETLIST_MASK   0xffff

// volatile struct gic_dist_map *const gic_dist = (volatile struct gic_dist_map *)(GICD_PPTR);

volatile struct gic_rdist_map *gic_rdist_map[CONFIG_MAX_NUM_NODES] = { 0 };
volatile struct gic_rdist_sgi_ppi_map *gic_rdist_sgi_ppi_map[CONFIG_MAX_NUM_NODES] = { 0 };

#ifdef CONFIG_ARCH_AARCH64
#define MPIDR_AFF0(x) (x & 0xff)
#define MPIDR_AFF1(x) ((x >> 8) & 0xff)
#define MPIDR_AFF2(x) ((x >> 16) & 0xff)
#define MPIDR_AFF3(x) ((x >> 32) & 0xff)
#else
#define MPIDR_AFF0(x) (x & 0xff)
#define MPIDR_AFF1(x) ((x >> 8) & 0xff)
#define MPIDR_AFF2(x) ((x >> 16) & 0xff)
#define MPIDR_AFF3(x) (0)
#endif
#define MPIDR_MT(x)   (x & BIT(24))
#define MPIDR_AFF_MASK(x) (x & 0xff00ffffff)

static word_t mpidr_map[CONFIG_MAX_NUM_NODES];

static inline uint64_t mpidr_to_gic_affinity(void)
{
    word_t mpidr = mpidr_map[CURRENT_CPU_INDEX()];
    return (uint64_t)MPIDR_AFF3(mpidr) << 32 | MPIDR_AFF2(mpidr) << 16 |
           MPIDR_AFF1(mpidr) << 8  | MPIDR_AFF0(mpidr);
}

/* Busy-Wait for completion of a distributor change */
/** DONT_TRANSLATE */
static uint32_t gicv3_do_busy_wait_for_rwp(volatile uint32_t *ctlr_addr)
{
    uint64_t gpt_cnt_ciel = GIC_DEADLINE_MS * TICKS_PER_MS;
    bool_t do_init = true;

    for (;;) {

        /* Check the value before reading the generic timer */
        uint32_t val = *ctlr_addr;
        if (!(val & GICD_CTLR_RWP)) {
            return 0;
        }

        uint64_t gpt_cnt_tval = 0;
        SYSTEM_READ_64(CNT_CT, gpt_cnt_tval);

        if (do_init) {
            gpt_cnt_ciel += gpt_cnt_tval;
            do_init = false;
        } else if (gpt_cnt_tval >= gpt_cnt_ciel) {
            printf("GICV3 RWP Timeout after %u ms\n", GIC_DEADLINE_MS);
            return 1;
        }
    }
    UNRECHABLE();
}

static void gicv3_enable_sre(void)
{
    uint32_t val = 0;

    /* ICC_SRE_EL1 */
    SYSTEM_READ_WORD(ICC_SRE_EL1, val);
    val |= GICC_SRE_EL1_SRE;

    SYSTEM_WRITE_WORD(ICC_SRE_EL1, val);
    isb();
}


BOOT_CODE static void dist_init(void)
{
    /* Disable GIC Distributor */
    gic_dist->ctlr = 0;
    gicv3_do_busy_wait_for_rwp(&gic_dist->ctlr);

    uint32_t type = gic_dist->typer;
    unsigned int nr_lines = GIC_REG_WIDTH * ((type & GICD_TYPE_LINESNR) + 1);

    /* Assume level-triggered */
    for (int i = SPI_START; i < nr_lines; i += 16) {
        gic_dist->icfgrn[(i / 16)] = 0;
    }

    /* Default priority for global interrupts */
    uint32_t priority = GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 |
                        GIC_PRI_IRQ << 8 | GIC_PRI_IRQ;
    for (int i = SPI_START; i < nr_lines; i += 4) {
        gic_dist->ipriorityrn[(i / 4)] = priority;
    }
    /* Disable and clear all global interrupts */
    for (int i = SPI_START; i < nr_lines; i += 32) {
        gic_dist->icenablern[(i / 32)] = IRQ_SET_ALL;
        gic_dist->icpendrn[(i / 32)] = IRQ_SET_ALL;
    }

    /* Turn on the distributor */
    gic_dist->ctlr = GICD_CTL_ENABLE | GICD_CTLR_ARE_NS | GICD_CTLR_ENABLE_G1NS | GICD_CTLR_ENABLE_G0;
    gicv3_do_busy_wait_for_rwp(&gic_dist->ctlr);

    /* Route all global IRQs to this CPU */
    uint64_t affinity = mpidr_to_gic_affinity();
    for (int i = SPI_START; i < nr_lines; i++) {
        gic_dist->iroutern[i - SPI_START] = affinity;
    }
}

BOOT_CODE static void gicr_init(void)
{
    /* Find redistributor for this core. */
    volatile struct gic_rdist_sgi_ppi_map *map = NULL;

    int core_id = CURRENT_CPU_INDEX();
    word_t mpidr = mpidr_map[core_id];
    word_t mpidr_typer = (MPIDR_AFF3(mpidr) << 24) | (MPIDR_AFF2(mpidr) << 16) |
                         (MPIDR_AFF1(mpidr) <<  8) | MPIDR_AFF0(mpidr);

    /*
     * Iterate through all redistributor interfaces looking for one that matches
     * our mpidr.
     */
    for (size_t offset = 0; offset < GICR_SIZE; offset += GICR_PER_CORE_SIZE) {
        volatile struct gic_rdist_map *gicr = (void *)((uintptr_t)GICR_PPTR + offset);
        if ((gicr->typer >> 32) != mpidr_typer) {
            continue;
        }
        if (gic_rdist_map[core_id] != NULL || gic_rdist_sgi_ppi_map[core_id] != NULL) {
            printf("GICv3: %s[%d] %p is not null\n",
                   gic_rdist_map[core_id] == NULL ? "gic_rdist_map" : "gic_rdist_sgi_ppi_map",
                   core_id,
                   gic_rdist_map[core_id] == NULL ? (void *)gic_rdist_map[core_id] : (void *)gic_rdist_sgi_ppi_map[core_id]);
            halt();
        }

        /*
         * GICR_WAKER should be Read-all-zeros in Non-secure world
         * and we expect redistributors to be alread awoken by an earlier loader.
         * However if we get a value back then something is probably wrong.
         */
        uint32_t val = gicr->waker;
        if (val & GICR_WAKER_ChildrenAsleep) {
            printf("GICv3: GICR_WAKER returned non-zero %x\n", val);
            halt();
        }

        gic_rdist_map[core_id] = gicr;
        map = (volatile struct gic_rdist_sgi_ppi_map *)((uintptr_t)gicr + RDIST_BANK_SZ);
        gic_rdist_sgi_ppi_map[core_id] = map;
        break;
    }

    if (!map) {
        printf("GICv3: GICR base for CPU %d %d %d %d (Logic ID %d) not found\n",
               (int)MPIDR_AFF3(mpidr), (int)MPIDR_AFF2(mpidr),
               (int)MPIDR_AFF1(mpidr), (int)MPIDR_AFF0(mpidr), core_id);
        halt();
    }

    /* Deactivate SGIs/PPIs */
    map->icactiver0 = ~0;

    /* Set priority on PPI and SGI interrupts */
    uint32_t priority = GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 |
                        GIC_PRI_IRQ << 8 | GIC_PRI_IRQ;
    for (int i = 0; i < SPI_START; i += 4) {
        map->ipriorityrn[i / 4] = priority;
    }

    /*
     * Disable all PPI interrupts, ensure all SGI interrupts are
     * enabled.
     */
    map->icenabler0 = 0xffff0000;
    map->isenabler0 = 0x0000ffff;

    /* Set ICFGR1 for PPIs as level-triggered */
    map->icfgr1 = 0x0;

    gicv3_do_busy_wait_for_rwp(&gic_rdist_map[CURRENT_CPU_INDEX()]->ctlr);
}

BOOT_CODE static void cpu_iface_init(void)
{
    /* Enable system registers */
    gicv3_enable_sre();

    /* No priority grouping: ICC_BPR1_EL1 */
    SYSTEM_WRITE_WORD(ICC_BPR1_EL1, 0);

    /* Set priority mask register: ICC_PMR_EL1 */
    SYSTEM_WRITE_WORD(ICC_PMR_EL1, DEFAULT_PMR_VALUE);

    /* EOI drops priority and deactivates the interrupt: ICC_CTLR_EL1 */
    uint32_t icc_ctlr = 0;
    SYSTEM_READ_WORD(ICC_CTLR_EL1, icc_ctlr);
    icc_ctlr &= ~GICC_CTLR_EL1_EOImode_drop;
    SYSTEM_WRITE_WORD(ICC_CTLR_EL1, icc_ctlr);

    /* Enable Group1 interrupts: ICC_IGRPEN1_EL1 */
    SYSTEM_WRITE_WORD(ICC_IGRPEN1_EL1, 1);

    /* Sync at once at the end of cpu interface configuration */
    isb();
}

void setIRQTrigger(irq_t irq, bool_t trigger)
{
    /* GICv3 has read-only GICR_ICFG0 for SGI with default value 0xaaaaaaaa,
     * and read-write GICR_ICFG1 for PPI with default 0x00000000.
     */
    word_t hw_irq = IRQT_TO_IRQ(irq);
    if (HW_IRQ_IS_SGI(hw_irq)) {
        return;
    }

    /* Update GICD_ICFGR<n>. Note that the interrupt should be disabled before
     * changing the field. This function assumes the caller has disabled the
     * interrupt.
     */
    volatile uint32_t *reg = HW_IRQ_IS_PPI(hw_irq) ? &(gic_rdist_sgi_ppi_map[IRQT_TO_CORE(irq)]->icfgr1)
                             : &(gic_dist->icfgrn[hw_irq >> 4]);
    int bit = ((hw_irq & 0xf) * 2);
    uint32_t icfgr = *reg;
    if (trigger) {
        icfgr |= (2 << bit);
    } else {
        icfgr &= ~(3 << bit);
    }
    *reg = icfgr;
    return;
}

BOOT_CODE void initIRQController(void)
{
    dist_init();
}

BOOT_CODE void cpu_initLocalIRQController(void)
{
    word_t mpidr = 0;
    SYSTEM_READ_WORD(MPIDR, mpidr);

    mpidr_map[CURRENT_CPU_INDEX()] = mpidr;

    gicr_init();
    cpu_iface_init();
}

#ifdef ENABLE_SMP_SUPPORT
#define MPIDR_MT(x)   (x & BIT(24))

void ipi_send_target(irq_t irq, word_t cpuTargetList)
{
    uint64_t sgi1r_base = ((word_t) IRQT_TO_IRQ(irq)) << ICC_SGI1R_INTID_SHIFT;
    word_t sgi1r[CONFIG_MAX_NUM_NODES];
    word_t last_aff1 = 0;

    for (word_t i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
        sgi1r[i] = 0;
        if (cpuTargetList & BIT(i)) {
            word_t mpidr = mpidr_map[i];
            word_t aff1 = MPIDR_AFF1(mpidr);
            word_t aff0 = MPIDR_AFF0(mpidr);
            // AFF1 is assumed to be contiguous and less than CONFIG_MAX_NUM_NODES.
            // The targets are grouped by AFF1.
            assert(aff1 >= 0 && aff1 < CONFIG_MAX_NUM_NODES);
            sgi1r[aff1] |= sgi1r_base | (aff1 << ICC_SGI1R_AFF1_SHIFT) | (1 << aff0);
            if (aff1 > last_aff1) {
                last_aff1 = aff1;
            }
        }
    }
    for (word_t i = 0; i <= last_aff1; i++) {
        if (sgi1r[i] != 0) {
            SYSTEM_WRITE_64(ICC_SGI1R_EL1, sgi1r[i]);
        }
    }
    isb();
}

void setIRQTarget(irq_t irq, seL4_Word target)
{
    if (IRQ_IS_PPI(irq)) {
        fail("PPI can't have designated target core\n");
        return;
    }

    word_t hw_irq = IRQT_TO_IRQ(irq);
    gic_dist->iroutern[hw_irq - SPI_START] = MPIDR_AFF_MASK(mpidr_map[target]);
}

#endif /* ENABLE_SMP_SUPPORT */

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT

word_t gic_vcpu_num_list_regs;

#endif /* End of CONFIG_ARM_HYPERVISOR_SUPPORT */
