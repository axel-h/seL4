/*
 * Copyright 2024, Codasip
 * Copyright 2021, HENSOLDT Cyber
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 * SBI provides an interface to read the time an set the timer interrupt. The
 * unterlying mechanism is usually that the CLINT contains a timer that SBI
 * is programming then. The register CLINT.mtime might be accessible, if no PMP
 * protection has been set up by SBI, so via CONFIG_RISCV_USE_CLINT_MTIME the
 * time can be read directly from there instead of doing a much slower SBI call.
 */

#pragma once

#include <config.h>
#include <types.h>
#include <mode/machine.h>
#include <arch/sbi.h>
#ifdef CONFIG_KERNEL_MCS
#include <arch/machine/timer.h>
#endif



#ifdef CONFIG_RISCV_USE_CLINT_MTIME

/*
 * All RISC-V platforms currently supported follow the same CLINT register
 * layout.
 */
#if CONFIG_WORD_SIZE == 32

#define CLINT_MTIME_OFFSET_LO 0xbff8
#define CLINT_MTIME_OFFSET_HI 0xbffc

static inline uint32_t riscv_read_clint_u32(word_t offset)
{
    return *(volatile uint32_t *)(CLINT_PPTR + offset);
}

static inline uint64_t riscv_read_clint_mtime(void)
{
    /*
     * Ensure that the time is correct if there is a rollover in the
     * high bits between reading the low and high bits.
     */
    uint32_t nH_prev = riscv_read_clint_u32(CLINT_MTIME_OFFSET_HI);
    uint32_t nL = riscv_read_clint_u32(CLINT_MTIME_OFFSET_LO);
    uint32_t nH = riscv_read_clint_u32(CLINT_MTIME_OFFSET_HI);
    if (nH_prev != nH) {
        nL = riscv_read_clint_u32(CLINT_MTIME_OFFSET_LO);
    }
    return (((uint64_t)nH) << 32) | nL;
}

#elif CONFIG_WORD_SIZE == 64

#define CLINT_MTIME_OFFSET 0xbff8

static inline uint64_t riscv_read_clint_u64(word_t offset)
{
    return *(volatile uint64_t *)(CLINT_PPTR + offset);
}

static inline uint64_t riscv_read_clint_mtime(void)
{
    return riscv_read_clint_u64(CLINT_MTIME_OFFSET);
}

#else
#error "unsupported CONFIG_WORD_SIZE"
#endif

#endif /* CONFIG_RISCV_USE_CLINT_MTIME */

static inline uint64_t getCurrentTime(void)
{
#ifdef CONFIG_RISCV_USE_CLINT_MTIME
    return riscv_read_clint_mtime();
#elif CONFIG_WORD_SIZE == 32
    return riscv_read_csr64cntr_time();
#elif CONFIG_WORD_SIZE == 64
    return riscv_read_csr_time();
#else
#error "CONFIG_WORD_SIZE"
#endif
}


#ifdef CONFIG_KERNEL_MCS

static inline PURE ticks_t getTimerPrecision(void)
{
    /* A timer running at above 1 MHz provides a micro second precission. If
     * more accuracy is needed, this is what can be configured on ARM:
     *   return usToTicks(TIMER_PRECISION) + TIMER_OVERHEAD_TICKS;
     */
    SEL4_COMPILE_ASSERT(timer_clock_at_least_1mhz, TIMER_CLOCK_HZ >= 1000000);
    return usToTicks(1);
}

/* set the next deadline irq - deadline is absolute */
static inline void setDeadline(ticks_t deadline)
{
    /* Setting the timer acknowledges any existing IRQs */
    sbi_set_timer(deadline);
}

/* ack previous deadline irq */
static inline void ackDeadlineIRQ(void)
{
    /* Nothing to be done, reprogramming the timer will clear the interrupt. */
}


#else /* not CONFIG_KERNEL_MCS */


/* When the SBI timer is used on RISC-V, there is the assumpton that it runs at
 * a frequency well above 1 MHz, so there are much more than 1000 ticks ber
 * milli scecond.
 */

#define TICKS_PER_MS (TIMER_CLOCK_HZ / MS_IN_S)
#define TIMER_RELOAD (TICKS_PER_MS * CONFIG_TIMER_TICK_MS)

static inline void resetTimer(void)
{
    uint64_t now = getCurrentTime();
    uint64_t target = now + TIMER_RELOAD;
    sbi_set_timer(target);
    uint64_t new_now = getCurrentTime();
    uint64_t delta = new_now - now;
    if (delta < TIMER_RELOAD) {
        return;
    }
    printf("Timer reset failed, %"PRIu64" (now) >= %"PRIu64"\n", new_now, target);
    halt();
}

BOOT_CODE static inline void initTimer(void)
{
#ifdef CONFIG_DEBUG_BUILD
    printf("Timer Info:\n");
    printf("  TIMER_CLOCK_HZ: %"PRIu64"\n", (uint64_t)TIMER_CLOCK_HZ);
    printf("  CONFIG_TIMER_TICK_MS: %"PRIu64"\n", (uint64_t)CONFIG_TIMER_TICK_MS);
    printf("  -> TICKS_PER_MS: %"PRIu64"\n", (uint64_t)TICKS_PER_MS);
    printf("  -> TIMER_RELOAD: %"PRIu64"\n", (uint64_t)TIMER_RELOAD);
#endif /* CONFIG_DEBUG_BUILD */
    uint64_t sum = 0;
    uint64_t cnt = 0;
    uint64_t now = getCurrentTime();
    uint64_t instret = riscv_read_instret();
    while (sum < TIMER_RELOAD) {
        sbi_set_timer(now + TIMER_RELOAD);
        uint64_t new_now = getCurrentTime();
        uint64_t delta = new_now - now;
        if (delta > TIMER_RELOAD) {
            uint64_t frac = TIMER_RELOAD / delta;
            printf("  Timer error: SBI timer set/read takes %"PRIu64" times TIMER_RELOAD\n", frac);
            halt();
        }
        now = new_now;
        sum += delta;
        cnt++;
    }
    instret = riscv_read_instret() - instret;
    printf("  TIMER_RELOAD allows calling SBI set/read %"PRIu64" times (instr %"PRIu64")\n", cnt, instret);


    /* initialising the SBI timer is like resetting it */
    resetTimer();
}

#endif /* [not] CONFIG_KERNEL_MCS */
