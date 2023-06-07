#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

from __future__ import annotations
from hardware.memory import Region

# "annotations" exists in __future__ since 3.7.0b1, but even in 3.10 the
# decision to make it mandatory has been postponed.
import sys
assert sys.version_info >= (3, 7)


class Config:
    ''' Abstract config class '''
    arch = 'unknown'

    def __init__(self, sel4arch, addrspace_max):
        self.sel4arch = sel4arch
        self.addrspace_max = addrspace_max

    def get_kernel_phys_align(self) -> int:
        ''' Used to align the base of physical memory. Returns alignment size in bits. '''
        return 0

    def get_bootloader_reserve(self) -> int:
        ''' Used to reserve a fixed amount of memory for the bootloader. Offsets
            the kernel load address by the amount returned in bytes. '''
        return 0

    def get_page_bits(self) -> int:
        ''' Get page size in bits for this arch '''
        return 12  # 4096-byte pages

    def get_smallest_kernel_object_alignment(self) -> int:
        return 4  # seL4_MinUntypedBits is 4 for all configurations

    def get_device_page_bits(self) -> int:
        ''' Get page size in bits for mapping devices for this arch '''
        return self.get_page_bits()

    def align_memory(self, regions: Set[Region]) -> List[Region], Set[Region], int:
        ''' Given a set of regions, sort them and align the first so that the
        ELF loader will be able to load the kernel into it. Will return the
        aligned memory region list, a set of any regions of memory that were
        aligned out and the physBase value that the kernel will use. memory
        region list, a set of any regions of memory that were aligned out and
        the physBase value that the kernel will use. '''
        regions = sorted(regions)
        return regions, set(), region[0].base


class ARMConfig(Config):
    ''' Config class for ARM '''
    arch = 'arm'
    SUPERSECTION_BITS = 24  # 2^24 = 16 MiByte

    def get_kernel_phys_align(self) -> int:
        ''' on ARM the ELF loader expects to be able to map a supersection page to load the kernel. '''
        return self.SUPERSECTION_BITS

    def align_memory(self, regions: Set[Region]) -> List[Region], Set[Region], int:
        ''' Arm wants physBase to be the physical load address of the kernel. '''
        regions = sorted(regions)
        extra_reserved = set()

        physBase = hardware.utils.align_up(region[0].base, align_bits)
        diff = physBase - region[0].base;
        if diff > 0:
            extra_reserved.add(region[0].cut_from_start(diff))

        return regions, extra_reserved, physBase


class RISCVConfig(Config):
    ''' Config class for RISCV '''
    arch = 'riscv'
    MEGAPAGE_BITS_RV32 = 22  # 2^22 = 4 MiByte
    MEGAPAGE_BITS_RV64 = 21  # 2^21 = 2 MiByte
    MEGA_PAGE_SIZE_RV64 = 2**MEGAPAGE_BITS_RV64

    def get_bootloader_reserve(self) -> int:
        ''' OpenSBI reserved the first 2 MiByte of physical memory on rv64,
        which is exactly a megapage. For rv32 we use the same value for now, as
        this seems to work nicely - even if this is just half of the 4 MiByte
        magepages that exist there. '''
        return self.MEGA_PAGE_SIZE_RV64

    def align_memory(self, regions: Set[Region]) -> List[Region], Set[Region], int:
        ''' Currently the RISC-V port expects physBase to be the address that the
        bootloader is loaded at. To be generalised in the future. '''
        regions = sorted(regions)
        extra_reserved = set()

        physBase = regions[0].base
        # reserve bootloader region
        l = self.get_bootloader_reserve()
        if l > 0:
            extra_reserved.add(regions[0].cut_from_start(l))

        return regions, extra_reserved, physBase

    def get_device_page_bits(self) -> int:
        ''' Get page size in bits for mapping devices for this arch '''
        if (self.sel4arch == 'riscv32'):
            # 4MiB device pages
            return self.MEGAPAGE_BITS_RV32
        elif (self.sel4arch == 'riscv64'):
            # 2MiB device pages for sv39 and sv48
            return self.MEGAPAGE_BITS_RV64
        raise ValueError('Unsupported sel4arch "{}" specified.'.format(self.sel4arch))


def get_arch_config(sel4arch: str, addrspace_max: int) -> Config:
    ''' Return an appropriate Config object for the given architecture '''
    if sel4arch in ['aarch32', 'aarch64', 'arm_hyp']:
        return ARMConfig(sel4arch, addrspace_max)
    elif sel4arch in ['riscv32', 'riscv64']:
        return RISCVConfig(sel4arch, addrspace_max)
    else:
        raise ValueError('Unsupported sel4arch "{}" specified.'.format(sel4arch))
