#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

from __future__ import annotations
import functools
from hardware.device import WrappedNode


@functools.total_ordering
class Region:
    ''' Represents a region of memory. '''

    def __init__(self, base: int, size: int, owner: WrappedNode = None):
        self.base = base
        self.size = size
        self.owner = owner

    @staticmethod
    def clone(other) -> Region:
        return Region(other.base, other.size)

    def __repr__(self) -> str:
        ''' Returns a string representation that is a valid Python expression
        that eval() can parse. '''
        return 'Region(base=0x{:x},size=0x{:x})'.format(self.base, self.size)

    def __str__(self) -> str:
        ''' Returns a string representation. '''
        return 'Region [0x{:x}..0x{:x}] ({:d} bytes)'.format(
            self.base,
            self.base + self.size - (1 if self.size > 0 else 0),
            self.size)

    def __eq__(self, other: Region) -> bool:
        return self.base == other.base and self.size == other.size

    def __ne__(self, other: Region) -> bool:
        # Needed only for py2.
        return not self.__eq__(other)

    def __gt__(self, other: Region) -> bool:
        return self.base > other.base

    def __hash__(self) -> bool:
        return hash((self.base, self.size))

    @staticmethod
    def from_range(start: int, end: int, owner: WrappedNode = None) -> Region:
        ''' create a region from a start/end rather than start/size '''
        if start > end:
            raise ValueError(
                'invalid rage start (0x{:x}) > end (0x{:x})'.format(start > end))
        return Region(start, end - start, owner)

    def overlaps(self, other: Region) -> bool:
        ''' returns True if this region overlaps the given region '''
        # Either our base is first, and to overlap our end must be > other.base
        # or other.base is first, and to overlap other's end must be > self.base
        return (self.base <= other.base and (self.base + self.size) > other.base) \
            or (other.base <= self.base and (other.base + other.size) > self.base)

    def reserve(self, excluded: Region) -> List[Region]:
        ''' returns an array of regions that represent this region
        minus the excluded range '''
        if not self.overlaps(excluded):
            return [Region(self.base, self.size, self.owner)]

        ret = []
        if self.base < excluded.base:
            # the first region is from our base to excluded.base
            ret.append(Region.from_range(self.base, excluded.base, self.owner))
            # skip the region from excluded.base - excluded.base + excluded.size
            # if there's anything left, add it.
            if (excluded.base + excluded.size) < (self.base + self.size):
                ret.append(Region.from_range(excluded.base + excluded.size,
                                             self.base + self.size, self.owner))
        else:  # self.base >= excluded.base
            # we skip the first chunk
            # we add what's left after the current chunk.
            if (self.base + self.size) > (excluded.base + excluded.size):
                ret.append(Region.from_range(excluded.base + excluded.size,
                                             self.base + self.size, self.owner))
        return ret

    def cut_from_start(self, size: int) -> Region:
        ''' Cut off a region from the start and return it, no owner is set.
        Adjust the original region's size.'''
        if size > self.size:
            raise ValueError(
                'can\'t cut off {} from start, region size is only {}'.format(
                    size, self.size))
        cut_reg = Region(self.base, size)
        self.base += size
        self.size -= size
        return cut_reg # this might be a region with a size of 0 bytes

    def cut_from_end(self, size: int) -> Region:
        ''' Cut off a region from the end and return it, no owner is set.
        Adjust the original region's start and size.'''
        if size > self.size:
            raise ValueError(
                'can\'t cut off {} from end, region size is only {}'.format(
                    size, self.size))
        if 0 == size:
            return None
        cut_reg = Region(self.base + self.size - size, size)
        self.size -= size
        return cut_reg # this might be a region with a size of 0 bytes
