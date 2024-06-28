#
# Copyright 2022, Axel Heider <axelheider@gmx.de>
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

#math(EXPR KernelPaddrUserTop "1 << 64")
set(KernelPhysAddressSpaceBits 64)

# This is not supported on ACME
#set(KernelHardwareDebugAPIUnsupported ON CACHE INTERNAL "")

add_sources(
    DEP "KernelArchACME"
    PREFIX src/arch/acme
    CFILES
        # c_traps.c
        # idle.c
        # api/faults.c
        # api/benchmark.c
        # kernel/boot.c
        # kernel/thread.c
        # kernel/vspace.c
        # machine/capdl.c
        # machine/hardware.c
        # machine/registerset.c
        # machine/io.c
        # machine/fpu.c
        # model/statedata.c
        # object/interrupt.c
        # object/objecttype.c
        # object/tcb.c
        # smp/ipi.c
    ASMFILES
        # head.S
        # traps.S
)

add_bf_source_old(
    "KernelArchACME"
    "structures.bf"
    "include/arch/acme"
    "arch/object"
)
