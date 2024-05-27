#
# Copyright 2024, Codasip GmbH
#
# SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(hobgoblin KernelPlatformHobgoblin PLAT_HOBGOBLIN KernelSel4ArchRiscV64)

if(KernelPlatformHobgoblin)
    declare_seL4_arch(riscv64)
    config_set(KernelRiscVPlatform RISCV_PLAT "${KernelPlatform}")
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 0)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
    set(KernelRiscvUseClintMtime OFF) # ToDo: check if this works
    list(APPEND KernelDTSList "tools/dts/${KernelPlatform}.dts")
    list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatform}.dts")
    declare_default_headers(
        TIMER_FREQUENCY 100000000 #  FPGA runs CLINT timer at 100 MHz
        MAX_IRQ 31 # DTS: soc/plic/riscv,ndev
        # INTERRUPT_CONTROLLER "drivers/irq/riscv_plic0.h"
    )
else()
    # ToDo: why are we doing this everywhere? Seems this should only be cleared
    #       once in the generic code and then the active platform may set this.
    #
    #    unset(KernelPlatformFirstHartID CACHE)
    #
endif()
