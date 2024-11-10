#
# Copyright 2020, DornerWorks
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "hifive"
    ARCH "riscv64"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformHifive"
    # C_DEFINE defaults to CONFIG_PLAT_HIFIVE
    # FLAGS: none
    # SOURCES: none
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformHifive)
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 1)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    declare_default_headers(
        TIMER_FREQUENCY 1000000
        MAX_IRQ 53
        INTERRUPT_CONTROLLER drivers/irq/riscv_plic0.h
    )
endif()
