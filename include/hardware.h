/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

/* This header file is shared by assembler and C code, thus it should contain
 * only definitions, but no actual C code.
 *
 * Each architecture defines a set of constants in #defines. These
 * constants describe the memory regions of the kernel's portion of the
 * address space including the physical memory window, the kernel ELF
 * region, and the device region.
 *
 *  - USER_TOP: The first address after the end of user memory
 *
 *  - PADDR_BASE: The first physical address mapped in the kernel's
 *    physical memory window.
 *  - PPTR_BASE: The first virtual address of the kernel's physical
 *    memory window.
 *  - PPTR_TOP: The first virtual address after the end of the kernel's
 *    physical memory window.
 *
 *  - KERNEL_ELF_PADDR_BASE: The first physical address used to map the
 *    initial kernel image. The kernel ELF is mapped contiguously
 *    starting at this address.
 *  - KERNEL_ELF_BASE: The first virtual address used to map the initial
 *    kernel image.
 *
 *  - KDEV_BASE: The first virtual address used to map devices.
 */

/* A reference to an object in the physical mapping window is translated to the
 * actual physical address (PPTR_TO_PADDR) and vice versa (PADDR_TO_PPTR).
 * This macro should be used to translate constants only. The wrapper functions
 * pptr_to_paddr() and paddr_to_pptr() should be used at runtime whenever
 * possible, they will  also handle the type conversion properly.
 */
#define PPTR_TO_PADDR(pptr) (PADDR_BASE + ((pptr) - PPTR_BASE))
#define PADDR_TO_PPTR(paddr) (PPTR_BASE  + ((paddr) - PADDR_BASE))

/* A virtual kernel image address is translated to a physical address. This
 * macro should be used to translate constants only and assumes the kernel image
 * is contiguous both virtually and physically. The wrapper function
 * pptr_to_paddr() should be used at runtime whenever possible, it also handles
 * the type conversion properly.
 */
#define KPPTR_TO_PADDR(va) (KERNEL_ELF_PADDR_BASE + ((va) - KERNEL_ELF_BASE))

/* KERNEL_ELF_PPTR_BASE is the location of the kernel image in the kernel's
 * physical mapping window. This can be equal to KERNEL_ELF_BASE if the kernel
 * is linked to match this mapping.
 */
#define KERNEL_ELF_PPTR_BASE PADDR_TO_PPTR(KERNEL_ELF_PADDR_BASE)

/* PADDR_TOP is the highest physical address that can be accessed via the
 * kernel's physical mapping window.
 */
#define PADDR_TOP PPTR_TO_PADDR(PPTR_TOP)

#include <mode/hardware.h>
