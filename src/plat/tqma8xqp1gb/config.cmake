#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "tqma8xqp1gb"
    ARCH "aarch64"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformTqma8xqp1gb"
    # C_DEFINE defaults to CONFIG_PLAT_TQMA8XQP1GB
    FLAGS
        "KernelArmCortexA35"
        "KernelArchArmV8a"
        "KernelArmGicV3"
        "KernelArmDisableWFIWFETraps"
    SOURCES
        "src/arch/arm/machine/gic_v3.c"
        "src/arch/arm/machine/l2c_nop.c"
    # FLAGS: none
    # SOURCES: none
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformTqma8xqp1gb)
    set(KernelArmVtimerUpdateVOffset OFF)
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    declare_default_headers(
        TIMER_FREQUENCY 8000000
        MAX_IRQ 511
        TIMER drivers/timer/arm_generic.h
        TIMER_OVERHEAD_TICKS 1
        INTERRUPT_CONTROLLER arch/machine/gic_v3.h
        NUM_PPI 32
        CLK_MAGIC 1llu
        CLK_SHIFT 3u
        KERNEL_WCET 10u
    )
endif()
