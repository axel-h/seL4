#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#
from typing import List, Set
from hardware.memory import Region


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

    def align_memory(self, regions: Set[Region]) -> List[Region]:
        ''' Given a set of regions, sort them and align the first so that the
        ELF loader will be able to load the kernel into it. Will return the
        aligned memory region list, a set of any regions of memory that were
        aligned out and the physBase value that the kernel will use. memory
        region list, a set of any regions of memory that were aligned out and
        the physBase value that the kernel will use. '''
        pass


class Config_ARM(Config):
    ''' Config class for ARM '''
    arch = 'arm'

    def __init__(self, sel4arch, addrspace_max):
        super().__init__(sel4arch, addrspace_max)
        self.SUPERSECTION_BITS = 25 if sel4arch == 'arm_hyp' else 24

    def get_kernel_phys_align(self) -> int:
        ''' On AArch32 the kernel requires at least super section alignment for physBase. '''
        return self.SUPERSECTION_BITS

    def align_memory(self, regions: Set[Region]) -> List[Region]:
        ''' Arm wants physBase to be the physical load address of the kernel. '''
        ret = sorted(regions)
        extra_reserved = set()

        new = ret[0].align_base(self.get_kernel_phys_align())
        resv = Region(ret[0].base, new.base - ret[0].base)
        extra_reserved.add(resv)
        ret[0] = new

        physBase = ret[0].base

        return ret, extra_reserved, physBase


class Config_RISCV(Config):
    ''' Abstract config class for RISC-V architecture'''
    arch = 'riscv'

    def get_bootloader_reserve(self) -> int:
        ''' OpenSBI reserved the first 2 MiByte of physical memory on rv64,
        which is exactly a megapage. For rv32 we use the same value for now, as
        this seems to work nicely - even if this is just half of the 4 MiByte
        magepages that exist there. '''
        return 2**21

    def align_memory(self, regions: Set[Region]) -> List[Region]:
        ''' Currently the RISC-V port expects physBase to be the address that the
        bootloader is loaded at. To be generalised in the future. '''
        ret = sorted(regions)
        extra_reserved = set()

        physBase = ret[0].base

        resv = Region(ret[0].base, self.get_bootloader_reserve())
        extra_reserved.add(resv)
        ret[0].base += self.get_bootloader_reserve()
        ret[0].size -= self.get_bootloader_reserve()

        return ret, extra_reserved, physBase


class Config_RISCV32(Config_RISCV):
    ''' Config class for RISC-V 32-bit '''
    MEGAPAGE_BITS = 22  # 2^22 = 4 MiByte

    def get_device_page_bits(self) -> int:
        ''' kernel devices are mapped into megapages '''
        return self.MEGAPAGE_BITS


class Config_RISCV64(Config_RISCV):
    ''' Config class for RISC-V 64-bit '''
    MEGAPAGE_BITS = 21  # 2^21 = 2 MiByte

    def get_device_page_bits(self) -> int:
        ''' kernel devices are mapped into megapages '''
        return self.MEGAPAGE_BITS


def get_arch_config(sel4arch: str, addrspace_max: int) -> Config:
    ''' Return an appropriate Config object for the given architecture '''
    for (ctor, arch_list) in [
        (Config_ARM,     ['aarch32', 'aarch64', 'arm_hyp']),
        (Config_RISCV32, ['riscv32']),
        (Config_RISCV64, ['riscv64']),
    ]:
        if sel4arch in arch_list:
            return ctor(sel4arch, addrspace_max)

    raise ValueError('Unsupported sel4arch "{}" specified.'.format(sel4arch))
