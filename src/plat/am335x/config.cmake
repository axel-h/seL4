#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "am335x"
    ARCH "aarch32"
    # use default board specific DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformAM335X"
    # C_DEFINE defaults to CONFIG_PLAT_AM335X
    FLAGS
        "KernelArmCortexA8"
        "KernelArchArmV7a"
    SOURCES
        "src/plat/am335x/machine/hardware.c"
        "src/plat/am335x/machine/l2cache.c"
    BOARDS # first is default
        "am335x-boneblack,KernelPlatformAM335XBoneBlack" # creates PLAT_AM335X_BONEBLACK
        "am335x-boneblue,KernelPlatformAM335XBoneBlue"   # creates PLAT_AM335X_BONEBLUE
)

if(KernelPlatformAM335X)

    set(KernelHardwareDebugAPIUnsupported ON CACHE INTERNAL "")

    if(KernelPlatformAM335XBoneBlack)
        add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    endif()
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")

    declare_default_headers(
        MAX_IRQ 127
        TIMER drivers/timer/am335x.h
        INTERRUPT_CONTROLLER
            drivers/irq/am335x.h
            #  DMTIMER 2-7 have programmable CLKSRC.
            #  Currently Kernel timer is DMTIMER4 using CLK_M_OSC.
        TIMER_FREQUENCY 24000000
        CLK_MAGIC 2863311531llu
        CLK_SHIFT 36u
        KERNEL_WCET 10u
    )

endif()
