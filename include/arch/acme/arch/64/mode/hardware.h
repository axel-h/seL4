/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <util.h>

/* last accessible virtual address in user space */
#define USER_TOP seL4_UserTop

/* The first physical address to map into the kernel's physical memory
 * window */
#define PADDR_BASE UL_CONST(0x0)

/* The base address in virtual memory to use for the 1:1 physical memory
 * mapping */
#define PPTR_BASE UL_CONST(0xFFFFFFC000000000)

/* Top of the physical memory window */
#define PPTR_TOP UL_CONST(0xFFFFFFFF80000000)

/* The physical memory address to use for mapping the kernel ELF */
/* This represents the physical address that the kernel image will be linked to. */
#define KERNEL_ELF_PADDR_BASE physBase

/* The base address in virtual memory to use for the kernel ELF mapping */
#define KERNEL_ELF_BASE (PPTR_TOP + (KERNEL_ELF_PADDR_BASE & MASK(30)))

/* The base address in virtual memory to use for the kernel device
 * mapping region. These are mapped in the kernel page table. */
#define KDEV_BASE UL_CONST(0xFFFFFFFFC0000000)

/* Place the kernel log buffer at the end of the kernel device page table */
#define KS_LOG_PPTR UL_CONST(0XFFFFFFFFFFE00000)
