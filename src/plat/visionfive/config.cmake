#
# Copyright 2023, Axel Heider
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(visionfive KernelPlatformVisionFive PLAT_VISIONFIVE KernelArchRiscV)

if(KernelPlatformVisionFive)

    # Notes:
    #   VifionFive v1: JH7100
    #   VifionFive v2: JH71110
    #
    # JH7100 Issues:
    #   - see https://github.com/starfive-tech/JH7100_Docs
    #   - problem with cacheability and coherence of I/O registers and buffers.
    #     - See https://www.reddit.com/r/RISCV/comments/13h12yv/visionfive_2_l2_cache_integrity/
    #     - Affected peripherals:  SDIO, GMAC, USB 3.0
    #     - must flush L2 cache to DRAM when DMA is used


    declare_seL4_arch(riscv64)
    config_set(KernelRiscVPlatform RISCV_PLAT "visionvive")
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 1)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
    list(APPEND KernelDTSList "tools/dts/visionfive-v1.dts")
    list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-visionfive.dts")
    declare_default_headers(
        TIMER_FREQUENCY 6250000 # from DTB: /cpus/timebase-frequency = <6250000>
        PLIC_MAX_NUM_INT 133 # from DTB: /soc/plic/riscv,ndev = <133>
        INTERRUPT_CONTROLLER drivers/irq/riscv_plic0.h
    )
else()
    unset(KernelPlatformFirstHartID CACHE)
endif()
