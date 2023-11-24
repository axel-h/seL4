#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
# Copyright 2021, HENSOLDT Cyber
# Copyright 2023, DornerWorks
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(
    "rocketchip"
    ARCH "riscv64"
    NO_DEFAULT_DTS # DTS is selected below
    CAMKE_VAR "KernelPlatformRocketchip"
    # C_DEFINE defaults to CONFIG_PLAT_ROCKETCHIP
    BOARDS # first is default
        "rocketchip-base,KernelPlatformRocketchipBase,PLAT_ROCKETCHIP_BASE"
        "rocketchip-zcu102,KernelPlatformRocketchipZCU102,PLAT_ROCKETCHIP_ZCU102"
)

if(KernelPlatformRocketchip)
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 0)
    list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelRiscVPlatform}.dts")
    # The Rocketchip-ZCU102 is a softcore instantiation that runs on the ZCU102's
    # FPGA fabric. Information on generating and running seL4 on the platform can
    # be found at https://docs.sel4.systems/Hardware/
    if(KernelPlatformRocketchipZCU102)
        # The rocket-fpga-zcu104 platform can be found at the following git repo:
        # https://github.com/bao-project/opensbi/tree/bao/rocket
        #
        # In order for this to function, please ensure the bao-project/opensbi
        # repo is added as a remote to the tools/opensbi project in the seL4 codebase
        config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "rocket-fpga-zcu104")
        # The zcu102 instantiation supports the PLIC and external interrupts
        declare_default_headers(
            TIMER_FREQUENCY 10000000
            MAX_IRQ 128
            INTERRUPT_CONTROLLER drivers/irq/riscv_plic0.h
        )
    elseif(KernelPlatformRocketchipBase)
        config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
        # This is an experimental platform that supports accessing peripherals, but
        # the status of support for external interrupts via a PLIC is unclear and
        # may differ depending on the version that is synthesized. Declaring no
        # interrupts and using the dummy PLIC driver seems the best option for now
        # to avoid confusion or even crashes.
        list(APPEND KernelDTSList "${CMAKE_CURRENT_LIST_DIR}/overlay-${KernelPlatformRocketchipBase}.dts")
        declare_default_headers(
            TIMER_FREQUENCY 10000000
            MAX_IRQ 0
            INTERRUPT_CONTROLLER drivers/irq/riscv_plic_dummy.h
        )
    else()
        message(FATAL_ERROR "unsupported rocketchip platform")
    endif()
endif()
