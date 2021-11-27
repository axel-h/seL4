#
# Copyright 2021, Axel Heider <axelheider@gmx.de>
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(nezha KernelPlatformNezha PLAT_NEZAH KernelArchRiscV)

# https://www.t-head.cn/product/c906?lang=en
#
# https://linux-sunxi.org/D1
# https://linux-sunxi.org/Allwinner_Nezha
# https://dl.linux-sunxi.org/D1/D1_Datasheet_V0.1_Draft_Version.pdf
# https://dl.linux-sunxi.org/D1/D1_User_Manual_V0.1_Draft_Version.pdf
#
# https://fedoraproject.org/wiki/Architectures/RISC-V/Allwinner
#
# https://blog.3mdeb.com/2021/2021-11-19-nezha-riscv-sbc-first-impression/
# https://ovsienko.info/D1/
#
# https://github.com/maquefel/licheerv-boot-build
#
# Nezha/D1
# SoC:   Allwinner D1
# CPU:   T-Head XuanTie C906, RV64GCV, 1.0Ghz, 32 KB I-cache + 32 KB D-cache
# DRAM:  512MiB/1GiB/2GiB DDR3 @ 792MHz, 2×H5TQ4G63EFR
# NAND:  256MB, MX35LF2GE4AD


if(KernelPlatformNezha)
    declare_seL4_arch(riscv64)
    config_set(KernelRiscVPlatform RISCV_PLAT "nezha")
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 0)
    config_set(KernelOpenSBIPlatform OPENSBI_PLATFORM "generic")
    list(APPEND KernelDTSList "tools/dts/nezha.dts")
    list(APPEND KernelDTSList "src/plat/nezha/overlay-nezha.dts")

    declare_default_headers(
        # The system runs with a 408 MHz PLL, but initially there might just be
        # the 24 MHz oscillator. See also DTB:
        #  - /cpus/timebase-frequency =  <24000000>
        #  - /cpus/cpu0/clock-frequency =  <24000000>
        #  - /osc24M/clock-frequency =  <24000000>
        TIMER_FREQUENCY 408000000
        # see DTS: soc/plic/riscv,ndev
        PLIC_MAX_NUM_INT 176
        # ToDo: This may not be SiFive PLCI compatible, as the description in
        # DTS:/soc/plic/compatible is "allwinner,sun20i-d1-plic", "thead,c900-plic";
        INTERRUPT_CONTROLLER drivers/irq/riscv_plic_dummy.h
    )
else()
    unset(KernelPlatformFirstHartID CACHE)
endif()
