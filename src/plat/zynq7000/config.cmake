#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "zynq7000"
    ARCH "aarch32"
    MACH "zynq"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformZynq7000"
    # C_DEFINE defaults to CONFIG_PLAT_ZYNQ7000
    FLAGS
        "KernelArmCortexA9"
        "KernelArchArmV7a"
    SOURCES
        "src/arch/arm/machine/l2c_310.c"
        "src/arch/arm/machine/gic_v2.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformZynq7000)

    if(KernelIsMCS)
        add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/mcs-overlay-${KernelPlatform}.dts")
        set(timer_file drivers/timer/arm_global.h)
    else()
        add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
        set(timer_file drivers/timer/arm_priv.h)
    endif()

    declare_default_headers(
        # This is the timer frequency that can pass tests (in particular
        # SCHED0011), but may not be the correct number.
        TIMER_FREQUENCY 320000000
        MAX_IRQ 92
        NUM_PPI 32
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        TIMER ${timer_file}
        CLK_SHIFT 40llu
        CLK_MAGIC 3435973837llu
        KERNEL_WCET 10llu
    )
endif()
