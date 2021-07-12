/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 * Copyright 2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

#include <config.h>
#define __ASSEMBLER__
#include <hardware.h>
#include <sel4/plat/api/constants.h>
#include <plat/machine/devices_gen.h>

KERNEL_OFFSET = KERNEL_ELF_BASE - KERNEL_ELF_PADDR_BASE;

SECTIONS
{
    . = KERNEL_ELF_BASE;

    .boot.text . : AT(ADDR(.boot.text) - KERNEL_OFFSET)
    {
        *(.boot.text)
    }
    .boot.rodata . : AT(ADDR(.boot.rodata) - KERNEL_OFFSET)
    {
        *(.boot.rodata)
    }
    .boot.data . : AT(ADDR(.boot.data) - KERNEL_OFFSET)
    {
        *(.boot.data)
    }
    . = ALIGN(4K);

    ki_boot_end = .;

    .text . : AT(ADDR(.text) - KERNEL_OFFSET)
    {
        /* Sit inside a large frame */
        . = ALIGN(4K);


        /* Standard kernel */
        *(.text)
    }

    /* Start of data section */
    _sdata = .;
    .sdata : {
        __global_pointer$ = . + 0x800;
        *(.sdata*)
    }
    .srodata : {
        *(.srodata*)
    }

    .rodata . : AT(ADDR(.rodata) - KERNEL_OFFSET)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data . : AT(ADDR(.data) - KERNEL_OFFSET)
    {
        *(.data)
    }

    .bss . : AT(ADDR(.bss) - KERNEL_OFFSET)
    {
        *(.bss)
        /* GCC before version 10 put uninitialized global variables into a
         * 'COMMON' section. If this section is not declared explicitly, these
         * variables end up at the end of the '.bss'. This tends to waste a page
         * because seL4 puts large aligned data at the end. In GCC 10
         * '-fno-common' was made the default, so handling 'COMMON' is no longer
         * needed. See also https://gcc.gnu.org/gcc-10/changes.html.
         */
        *(COMMON)
        *(.sbss)

        /* 4k breakpoint stack */
        _breakpoint_stack_bottom = .;
        . = . + 4K;
        _breakpoint_stack_top = .;

        /* large data such as the globals frame and global PD */
        *(.bss.aligned)
    }

    . = ALIGN(4K);
    ki_end = .;
}
