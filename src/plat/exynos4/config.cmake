#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "exynos4"
    ARCH "aarch32"
    MACH "exynos"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformExynos4"
    # C_DEFINE defaults to CONFIG_PLAT_EXYNOS4
    FLAGS
        "KernelArmCortexA9"
        "KernelArchArmV7a"
    SOURCES
        "src/arch/arm/machine/l2c_310.c"
        "src/arch/arm/machine/gic_v2.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformExynos4)

    list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")

    declare_default_headers(
        TIMER_FREQUENCY 24000000
        MAX_IRQ 159
        NUM_PPI 32
        TIMER drivers/timer/exynos4412-mct.h
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        KERNEL_WCET 10u
        CLK_MAGIC 2863311531llu
        CLK_SHIFT 36u
    )

endif()
