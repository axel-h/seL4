#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)


declare_platform(
    "zynqmp"
    ARCH "aarch64" "aarch32" "arm_hyp" # default is first (aarch64)
    MACH "zynq"
    NO_DEFAULT_DTS # DTS is selected below
    CAMKE_VAR "KernelPlatformZynqmp"
    # C_DEFINE defaults to CONFIG_PLAT_ZYNQMP
    FLAGS
        "KernelArmCortexA53"
        "KernelArchArmV8a"
    SOURCES
        "src/arch/arm/machine/gic_v2.c"
        "src/arch/arm/machine/l2c_nop.c"
    BOARDS # first is default
        "zcu102,KernelPlatformZynqmpZcu102,PLAT_ZYNQMP_ZCU102"
        "ultra96,KernelPlatformZynqmpUltra96,PLAT_ZYNQMP_ULTRA96"
        "ultra96v2,KernelPlatformZynqmpUltra96v2,PLAT_ZYNQMP_ULTRA96V2"
)


if(KernelPlatformZynqmp)

    set(KernelHardwareDebugAPIUnsupported ON CACHE INTERNAL "")

    if(KernelPlatformZynqmpUltra96v2 OR KernelPlatformZynqmpUltra96)
        add_platform_dts("tools/dts/${KernelBoard}.dts")
    elseif(KernelPlatformZynqmpZcu102)
        # The default board uses the platform name for the DTS and not the name
        # of the actual board.
        add_platform_dts("tools/dts/${KernelPlatform}.dts")
    else()
        message(FATAL_ERROR "unknown platform")
    endif()

    if(KernelSel4ArchAarch32)
        add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}32.dts")
    elseif(KernelSel4ArchAarch64)
        add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    else()
        message(FATAL_ERROR "unsupported KernelSel4Arch")
    endif()

    declare_default_headers(
        TIMER_FREQUENCY 100000000
        MAX_IRQ 187
        NUM_PPI 32
        TIMER drivers/timer/arm_generic.h
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        CLK_MAGIC 1374389535llu
        CLK_SHIFT 37u
        KERNEL_WCET 10u
    )

endif()
