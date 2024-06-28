#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "omap3"
    ARCH "aarch32"
    MACH "omap"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformOMAP3"
    # C_DEFINE defaults to CONFIG_PLAT_OMAP3
    FLAGS
        "KernelArmCortexA8"
        "KernelArchArmV7a"
    SOURCES
        "src/plat/omap3/machine/hardware.c"
        "src/plat/omap3/machine/l2cache.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformOMAP3)

    set(KernelHardwareDebugAPIUnsupported ON CACHE INTERNAL "")

    list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")

    declare_default_headers(
        TIMER_FREQUENCY 13000000
        MAX_IRQ 95
        INTERRUPT_CONTROLLER drivers/irq/omap3.h
        TIMER drivers/timer/omap3430.h
        CLK_MAGIC 1321528399llu
        CLK_SHIFT 34u
        KERNEL_WCET 10u
    )

endif()
