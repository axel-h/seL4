/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <config.h>
#include <mode/smp/ipi.h>
#include <smp/lock.h>
#include <util.h>

#ifdef ENABLE_SMP_SUPPORT

/* the remote call being requested */
static volatile IpiRemoteCall_t  remoteCall;

static inline void set_ipi(word_t core_id, irq_t ipi)
{
    assert(idx < core_id);
    coreMap.core[core_id].ipi = ipi
}

static inline irq_t get_ipi(word_t core_id)
{
    assert(idx < core_id);
    return coreMap.core[core_id].ipi;
}

static inline void clear_current_ipi(irq_t ipi)
{
    set_ipi(getCurrentCPUIndex(), irqInvalid)
}

static inline irq_t get_current_ipi(void)
{
    return get_ipi(getCurrentCPUIndex());
}

static inline void init_ipi_args(IpiRemoteCall_t func,
                                 word_t data1, word_t data2, word_t data3,
                                 word_t mask)
{
    remoteCall = func;
    ipi_args[0] = data1;
    ipi_args[1] = data2;
    ipi_args[2] = data3;

    /* get number of cores involved in this IPI */
    totalCoreBarrier = popcountl(mask);
}

static void handleRemoteCall(IpiRemoteCall_t call, word_t arg0,
                             word_t arg1, word_t arg2, bool_t irqPath)
{
    /* we gets spurious irq_remote_call_ipi calls, e.g. when handling IPI
     * in lock while hardware IPI is pending. Guard against spurious IPIs! */
    if (clh_is_ipi_pending(getCurrentCPUIndex())) {
        switch ((IpiRemoteCall_t)call) {
        case IpiRemoteCall_Stall:
            ipiStallCoreCallback(irqPath);
            break;

#ifdef CONFIG_HAVE_FPU
        case IpiRemoteCall_switchFpuOwner:
            switchLocalFpuOwner((user_fpu_state_t *)arg0);
            break;
#endif /* CONFIG_HAVE_FPU */

        default:
            fail("Invalid remote call");
            break;
        }

        big_kernel_lock.node_owners[getCurrentCPUIndex()].ipi = 0;
        clear_current_ipi();
        ipi_wait(totalCoreBarrier);
    }
}

void ipi_send_mask(irq_t ipi, word_t mask, bool_t isBlocking)
{

    generic_ipi_send_mask(ipi, mask, isBlocking);
}

irq_t ipi_get_irq(void)
{
    irq_t ipi = get_current_ipi();
    assert(!(ipi == irqInvalid && big_kernel_lock.node_owners[getCurrentCPUIndex()].ipi == 1));
    return ipi;
}

void ipi_clear_irq(irq_t irq)
{
    clear_current_ipi();
}

/* this function is called with a single hart id. */
void ipi_send_target(irq_t irq, word_t hart_id)
{
    printf("ipi_send_target to hart_id %"SEL4_PRIu_word")\n", hart_id);

    word_t hart_mask = BIT(hart_id);
    word_t core_id = hartIDToCoreID(hart_id);
    assert(core_id < CONFIG_MAX_NUM_NODES);

    assert((get_ipi(core_id) == irqInvalid) || (get_ipi(core_id) == irq_reschedule_ipi) ||
           (get_ipi(core_id) == irq_remote_call_ipi && big_kernel_lock.node_owners[core_id].ipi == 0));

    set_ipi(core_id, irq);
    fence_rw_rw();
    sbi_send_ipi(hart_mask);
}

#endif
