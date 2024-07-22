#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "allwinnerA20"
    ARCH "aarch32"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformAllwinnerA20"
    # C_DEFINE defaults to CONFIG_PLAT_ALLWINNERA20
    FLAGS
        "KernelArmCortexA7"
        "KernelArchArmV7a"
    SOURCES
        "src/plat/allwinnerA20/machine/l2cache.c"
        "src/arch/arm/machine/gic_v2.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformAllwinnerA20)
    # MCS is not supported on allwinnerA20. It requires a timer driver that
    # implements the tickless programming requirements.
    set(KernelPlatformSupportsMCS OFF)

    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")

    declare_default_headers(
        TIMER_FREQUENCY 24000000
        MAX_IRQ 122
        NUM_PPI 32
        TIMER drivers/timer/arm_generic.h
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
    )
endif()
