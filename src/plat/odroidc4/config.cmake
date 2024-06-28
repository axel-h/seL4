#
# Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "odroidc4"
    ARCH "aarch64"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformOdroidc4"
    # C_DEFINE defaults to CONFIG_PLAT_ODROIDC4
    FLAGS
        "KernelArmCortexA55"
        "KernelArchArmV8a"
    SOURCES
        "src/arch/arm/machine/gic_v2.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformOdroidc4)
    set(KernelArmMachFeatureModifiers "+crc" CACHE INTERNAL "")
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    # MAX_IRQ is based on the section 7.10.2 of the S905X3 SoC manual
    declare_default_headers(
        TIMER_FREQUENCY 24000000
        MAX_IRQ 255
        NUM_PPI 32
        TIMER drivers/timer/arm_generic.h
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        CLK_MAGIC 375299969u
        CLK_SHIFT 53u
        KERNEL_WCET 10u
        TIMER_PRECISION 1u
    )
endif()
