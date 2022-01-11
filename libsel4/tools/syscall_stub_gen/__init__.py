#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
# Copyright (c) 2022 Antmicro
#
# SPDX-License-Identifier: BSD-2-Clause
#

# This file allows the the libsel4 tools directory to be imported as
# a python module.

from . import generator, generator_c, generator_rust, syscall_stub_gen

# for backwards compatibility
Architecture = syscall_stub_gen.Architecture
CapType = generator_c.Generator_C.CapType
MAX_MESSAGE_LENGTH = generator.MAX_MESSAGE_LENGTH
parse_xml_file = generator.parse_xml_file
get_parameter_positions = generator.get_parameter_positions
generate_param_list = generator_c.Generator_C.generate_param_list


def init_data_types(wordsize):
    return generator.init_data_types(
        wordsize,
        generator_c.BitFieldType,
        generator_c.CapType,
        generator_c.Type)


def init_arch_types(wordsize):
    return generator.init_arch_types(
        wordsize,
        generator_c.CapType,
        generator_c.StructType,
        generator_c.Type
    )


def generate_result_struct(interface_name, method_name, output_params):
    class Mock():
        pass
    mockobj = Mock()
    mockobj.contents = []
    generator_c.Generator_C._gen_result_struct(mockobj, interface_name, method_name, output_params)
    return mockobj.contents
