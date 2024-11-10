#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "tx2"
    ARCH "aarch64"
    MACH "nvidia"
    # use default DTS at tools/dts/<board-name>.dts
    CAMKE_VAR "KernelPlatformTx2"
    # C_DEFINE defaults to CONFIG_PLAT_TX2
    FLAGS
        "KernelArmCortexA57"
        "KernelArchArmV8a"
        "KernelAArch64SErrorIgnore"
    SOURCES
        "src/arch/arm/machine/gic_v2.c"
        "src/arch/arm/machine/l2c_nop.c"
    # BOARDS: there is just one board, it defaults to the platform name
)

if(KernelPlatformTx2)
    # Note: If we enable the Denver 2 cores, which are 40-bit PA,
    # the 44-bit PA for Cortex-A57 cores would need to be downgraded to 40bit.
    add_platform_dts("${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    declare_default_headers(
        TIMER_FREQUENCY 31250000
        MAX_IRQ 383
        INTERRUPT_CONTROLLER arch/machine/gic_v2.h
        NUM_PPI 32
        TIMER drivers/timer/arm_generic.h
        CLK_SHIFT 57u
        CLK_MAGIC 4611686019u
        KERNEL_WCET 10u
        SMMU drivers/smmu/smmuv2.h
        MAX_SID 128
        MAX_CB 64
    )
endif()
