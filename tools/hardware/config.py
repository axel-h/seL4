#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#


class Config:
    ''' config base class '''
    arch = 'unknown'

    def __init__(self, phys_addr_space_bits):
        self.phys_addr_space_bits = phys_addr_space_bits

    def get_kernel_phys_align(self) -> int:
        '''
        Returns alignment size in 2^n bits of the kernel image.
        '''
        # There are no special alignment needs by default.
        return 0

    def get_bootloader_reserve(self) -> int:
        ''' Used to reserve a fixed amount of memory for the bootloader. Offsets
            the kernel load address by the amount returned in bytes. '''
        return 0

    def get_page_bits(self) -> int:
        '''
        Returns the page size in 2^n bits
        '''
        # Default to 4 KiByte (2^12) pages.
        return 12

    def get_device_page_bits(self) -> int:
        '''
        Return the page size in 2^n bits for mapping devices
        '''
        # By default, this is the same as the page size
        raise self.get_page_bits()

    def get_kernel_phys_addr_space_bits(self) -> int:
        '''
        Return the physical address space in 2^n bits.
        '''
        return self.phys_addr_space_bits


class ARMConfig(Config):
    ''' Config class for ARM '''
    arch = 'arm'

    def get_kernel_phys_align(self) -> int:
        # the ELF loader expects to be able to map a 16 MiByte (2^24)
        # "supersection" pages to load the kernel.
        return 24


class RISCVConfig(Config):
    ''' Config class for RISC-V '''
    arch = 'riscv'

    def get_bootloader_reserve(self) -> int:
        ''' on RISC-V OpenSBI is loaded at the start
        of physical memory. Mark it as unavailable. '''
        return self.MEGA_PAGE_SIZE

    def get_device_page_bits(self) -> int:
        # rv32 uses 4 MiByte (2^22) device pages, rv39 and rv48 use 2 MiByte
        # (2^21) "megapages" device pages
        return 22 if (self.phys_addr_space_bits <= 32) else 21


def get_arch_config(arch: str, phys_addr_space_bits: int) -> Config:
    ''' Return an appropriate Config object for the given architecture '''
    if arch == 'arm':
        return ARMConfig(phys_addr_space_bits)
    elif arch == 'riscv':
        return RISCVConfig(phys_addr_space_bits)
    raise ValueError('Unsupported arch "{}" specified.'.format(arch))
