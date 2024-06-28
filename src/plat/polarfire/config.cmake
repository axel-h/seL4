#
# Copyright 2020, DornerWorks
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "polarfire"
    ARCH "riscv64"
    NO_DEFAULT_DTS # there is no "tools/dts/<board-name>.dts"
    CAMKE_VAR "KernelPlatformPolarfire"
    # C_DEFINE defaults to CONFIG_PLAT_POLARFIRE
    # FLAGS: none
    # SOURCES: none
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformPolarfire)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 1)
    list(
        APPEND
        KernelDTSList
        "tools/dts/mpfs_icicle.dts"
        "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts"
    )
    declare_default_headers(
        TIMER_FREQUENCY 1000000
        MAX_IRQ 186
        INTERRUPT_CONTROLLER drivers/irq/riscv_plic0.h
    )

endif()
