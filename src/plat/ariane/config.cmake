#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
# Copyright 2021, HENSOLDT Cyber
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "ariane"
    ARCH "riscv64"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformAriane"
    # C_DEFINE defaults to CONFIG_PLAT_ARIANE
    # FLAGS: none
    # SOURCES: none
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformAriane)

    config_set(KernelPlatformFirstHartID FIRST_HART_ID 0)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "fpga/ariane")
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    declare_default_headers(
        MAX_IRQ 30
        TIMER_FREQUENCY 25000000
        INTERRUPT_CONTROLLER drivers/irq/riscv_plic0.h
    )

endif()
