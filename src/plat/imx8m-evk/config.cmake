#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
# Copyright 2022, Capgemini Engineering
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "imx8mq-evk"
    ARCH "aarch64" "aarch32"
    MACH "imx"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformImx8mq-evk"
    C_DEFINE "PLAT_IMX8MQ_EVK"
    FLAGS
        "KernelArmCortexA53"
        "KernelArchArmV8a"
        "KernelArmGicV3"
    SOURCES
        "src/arch/arm/machine/gic_v3.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

declare_platform(
    "imx8mm-evk"
    ARCH "aarch64" "aarch32"
    MACH "imx"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformImx8mm-evk"
    C_DEFINE "PLAT_IMX8MM_EVK"
    FLAGS
        "KernelArmCortexA53"
        "KernelArchArmV8a"
        "KernelArmGicV3"
    SOURCES
        "src/arch/arm/machine/gic_v3.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

declare_platform(
    "imx8mp-evk"
    ARCH "aarch64" "aarch32"
    MACH "imx"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformImx8mp-evk"
    C_DEFINE "PLAT_IMX8MP_EVK"
    FLAGS
        "KernelArmCortexA53"
        "KernelArchArmV8a"
        "KernelArmGicV3"
    SOURCES
        "src/arch/arm/machine/gic_v3.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformImx8mq-evk OR KernelPlatformImx8mm-evk OR KernelPlatformImx8mp-evk)
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    if(KernelSel4ArchAarch32)
        add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-imx8m-32bit.dts")
    endif()

    if(KernelPlatformImx8mq-evk)
        config_set(KernelPlatImx8mq PLAT_IMX8MQ ON)
    endif()

    if(KernelPlatformImx8mp-evk)
        # The i.MX 8M Plus SoC has higher interrupt numbers than the 8M Mini and the 8M Quad
        set(IMX8M_MAX_IRQ 192 CACHE INTERNAL "")
    else()
        set(IMX8M_MAX_IRQ 160 CACHE INTERNAL "")
    endif()
    declare_default_headers(
        TIMER_FREQUENCY 8000000
        MAX_IRQ ${IMX8M_MAX_IRQ}
        TIMER drivers/timer/arm_generic.h
        INTERRUPT_CONTROLLER arch/machine/gic_v3.h
        NUM_PPI 32
        CLK_MAGIC 1llu
        CLK_SHIFT 3u
        KERNEL_WCET 10u
    )

endif()
