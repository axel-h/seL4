#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "tx1"
    ARCH "aarch64"
    MACH "nvidia"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformTx1"
    # C_DEFINE defaults to CONFIG_PLAT_TX1
    FLAGS
        "KernelArmCortexA57"
        "KernelArchArmV8a"
    SOURCES
        "src/arch/arm/machine/gic_v2.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformTx1)

    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")

    declare_default_headers(
        TIMER_FREQUENCY 12000000
        MAX_IRQ 224
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        NUM_PPI 32
        TIMER drivers/timer/arm_generic.h
        CLK_MAGIC 2863311531llu
        CLK_SHIFT 35u
        KERNEL_WCET 10u
    )

endif()
