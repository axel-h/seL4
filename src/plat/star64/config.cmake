#
# Copyright 2023, Ivan Velickovic
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "star64" # actually, it's a JH7110 based platform
    ARCH "riscv64"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformStar64"
    # C_DEFINE defaults to CONFIG_PLAT_STAR64
    # FLAGS: none
    # SOURCES: none
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformStar64)

    # The JH7110 SoC contains the SiFive U74-MC core complex. This has four U74
    # cores and one S7 core (which has a hart ID of 0). The first U74 core has
    # a hart ID of 1.
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 1)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
    # Note that by default the kernel is configured for the 4GB Star64 model.
    list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    # The value for TIMER_FREQUENCY is from the "timebase-frequency" field on
    # the "cpus" node in the Star64 device tree.
    # The value for MAX_IRQ comes from the DTS "plic" node which says
    # "riscv,ndev = <0x88>".
    declare_default_headers(
        TIMER_FREQUENCY 4000000
        MAX_IRQ 136
        INTERRUPT_CONTROLLER drivers/irq/riscv_plic0.h
    )

endif()
