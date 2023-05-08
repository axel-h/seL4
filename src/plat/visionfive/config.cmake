#
# Copyright 2023, Axel Heider
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(visionfive KernelPlatformVisionFive PLAT_VISIONFIVE KernelArchRiscV)

if(KernelPlatformVisionFive)
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
