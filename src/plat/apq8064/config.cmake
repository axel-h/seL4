#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "apq8064"
    ARCH "aarch32"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformAPQ8064"
    # C_DEFINE defaults to CONFIG_PLAT_APQ8064
    FLAGS
        "KernelArmCortexA15"
        "KernelArchArmV7a"
        "KernelArchArmV7ve"
    SOURCES
        "src/arch/arm/machine/gic_v2.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformAPQ8064)

    # MCS is not supported on apq8064. It requires a timer driver that
    # implements the tickless programming requirements.
    set(KernelPlatformSupportsMCS OFF)

    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")

    declare_default_headers(
        TIMER_FREQUENCY 7000000
        MAX_IRQ 283
        NUM_PPI 32
        TIMER drivers/timer/arm_generic.h
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
    )
endif()
