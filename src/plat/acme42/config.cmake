#
# Copyright 2022, Axel Heider <axelheider@gmx.de>
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(acme42 KernelPlatformAcme42 PLAT_ACME KernelSel4ArchACME64)

if(KernelPlatformAcme42)

    declare_seL4_arch(acme64)

    # ToDo: make a nice device tree
    list(APPEND KernelDTSList "tools/dts/rockpro64.dts")
    list(APPEND KernelDTSList "src/plat/rockpro64/overlay-rockpro64.dts")

    declare_default_headers(
        TIMER_FREQUENCY 24000000
        MAX_IRQ 10
        KERNEL_WCET 10u
        INTERRUPT_CONTROLLER arch/machine/acme_int_ctrl.h
    )
endif()

#add_sources(
#    DEP "KernelPlatformAcme42"
#    CFILES src/arch/arm/machine/gic_v3.c src/arch/arm/machine/l2c_nop.c
#)
