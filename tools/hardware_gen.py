#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

import collections
import argparse
import logging
import yaml
import hardware
import hardware.outputs


# Create an sorted dict. With Python 3.7, dicts are guaranteed to preserve the
# order, older version require using OrderedDict explicitly.
OUTPUTS = collections.OrderedDict(sorted({
    'c_header': hardware.outputs.c_header,
    'compat_strings': hardware.outputs.compat_strings,
    'elfloader': hardware.outputs.elfloader,
    'yaml': hardware.outputs.yaml,
    'json': hardware.outputs.json,
}.items()))


def validate_rules(rules, schema):
    ''' Try and validate a hardware rules file against a schema.
        If jsonschema is not installed, succeed with a warning. '''
    try:
        from jsonschema import validate
        return validate(rules, schema)
    except ImportError:
        logging.warning('Skipping hardware YAML validation; `pip install jsonschema` to validate')
        return True


def hardware_gen(args: argparse.Namespace):
    ''' Parse the DT and hardware config YAML and run each
    selected output method. '''
    cfg = hardware.config.get_arch_config(args.sel4arch, args.addrspace_max)
    parsed_dt = hardware.fdt.FdtParser(args.dtb)
    rules = yaml.load(args.hardware_config, Loader=yaml.FullLoader)
    schema = yaml.load(args.hardware_schema, Loader=yaml.FullLoader)
    validate_rules(rules, schema)
    hw_yaml = hardware.utils.rule.HardwareYaml(rules, cfg)

    arg_dict = vars(args)
    for key, task in OUTPUTS.items():
        if arg_dict[key]:
            task.run(parsed_dt, hw_yaml, cfg, args)

def parse_args():
    parser = argparse.ArgumentParser(
        description='transform device tree input to seL4 build configuration artefacts'
    )

    parser.add_argument('--dtb', help='device tree blob to parse for generation',
                        required=True, type=argparse.FileType('rb'))
    parser.add_argument('--hardware-config', help='YAML file containing configuration for kernel devices',
                        required=True, type=argparse.FileType('r'))
    parser.add_argument('--hardware-schema', help='YAML file containing schema for hardware config',
                        required=True, type=argparse.FileType('r'))
    parser.add_argument('--sel4arch', help='seL4 architecture to generate for',
                        required=True)
    parser.add_argument('--addrspace-max',
                        help='maximum address that is available as device untyped', type=int, default=32)
    parser.add_argument('--enable-profiling', help='enable profiling', action='store_true')

    ''' Add arguments for each output type. '''
    for key, task in OUTPUTS.items():
        name = key.replace('_', '-')
        group = parser.add_argument_group(f'{name} pass')
        group.add_argument(f'--{name}', help=task.__doc__.strip(), action='store_true')
        task.add_args(group)

    args = parser.parse_args()

    return args

def main():
    args = parse_args()
    if args.enable_profiling:
        import cProfile
        with cProfile.Profile() as pr:
            hardware_gen(args)
            pr.print_stats(sort='cumtime')
    else:
        hardware_gen(args)

if __name__ == '__main__':
    main()
