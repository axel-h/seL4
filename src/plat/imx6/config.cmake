#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
# Copyright 2020, HENSOLDT Cyber GmbH
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "imx6"
    ARCH "aarch32"
    MACH "imx"
    # use default DTS at tools/dts/<board-name>.dts
    # The custom CAMKE_VAR is also used in these files:
    # - seL4/elfloader-tool/CMakeLists.txt
    # - libplatsupport/CMakeLists.txt
    CAMKE_VAR "KernelPlatImx6"
    PLAT_CAMKE_VARS # all disabled by default, must be enabled explicitly
        "KernelPlatImx6dq"
        "KernelPlatImx6sx"
    FLAGS
        "KernelArmCortexA9"
        "KernelArchArmV7a"
    SOURCES
        "src/arch/arm/machine/l2c_310.c"
        "src/arch/arm/machine/gic_v2.c"
    BOARDS # first is default
        "sabre,KernelPlatformSabre" # creates PLAT_SABRE
        "wandq,KernelPlatformWandQ" # creates PLAT_WANDQ
        "nitrogen6sx,KernelPlatformNitrogen6SX" # creates PLAT_NITROGEN6SX"
)

if(KernelPlatImx6)

    if(KernelPlatformSabre)
        config_set(KernelPlatImx6dq PLAT_IMX6DQ ON)

    elseif(KernelPlatformWandQ)
        config_set(KernelPlatImx6dq PLAT_IMX6DQ ON)

    elseif(KernelPlatformNitrogen6SX)
        config_set(KernelPlatImx6sx PLAT_IMX6SX ON)

    else()
        message(FATAL_ERROR "invalid i.MX6 board")
    endif()

    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelBoard}.dts")

    if(KernelIsMCS)
        if(KernelPlatformNitrogen6SX)
            add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/mcs-overlay-${KernelBoard}.dts")
        elseif(KernelPlatformSabre OR KernelPlatformWandQ)
            add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/mcs-overlay-${KernelPlatform}.dts")
        else()
            message(FATAL_ERROR "invalid i.MX6 board")
        endif()
        set(timer_file drivers/timer/arm_global.h)
    else()
        set(timer_file drivers/timer/arm_priv.h)
    endif()

    declare_default_headers(
        TIMER_FREQUENCY 498000000
        MAX_IRQ 159
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        NUM_PPI 32
        TIMER ${timer_file}
        CLK_SHIFT 41llu
        CLK_MAGIC 4415709349llu
        KERNEL_WCET 10llu
        TIMER_PRECISION 2u
    )

endif()
