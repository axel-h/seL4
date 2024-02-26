#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

''' generate a header file for the elfloader from a device tree '''

import argparse
import builtins
import logging
import pyfdt.pyfdt

from jinja2 import Environment, BaseLoader
from typing import Union

from hardware import config, device, fdt
from hardware.utils import cpu, memory, rule


HEADER_TEMPLATE = '''/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
 * This file is autogenerated by kernel/tools/hardware_gen.py
 */

#pragma once

#include <types.h>
{% if uses_psci %}
#include <psci.h>
{% endif %}

#define MAX_NUM_REGIONS {{ max_reg }}

struct elfloader_driver;

struct elfloader_device {
    const char *compat;
    volatile void *region_bases[MAX_NUM_REGIONS];
    struct elfloader_driver *drv;
};

struct elfloader_cpu {
    const char *compat;
    const char *enable_method;
    word_t cpu_id;
    word_t extra_data;
};

#ifdef DRIVER_COMMON
struct elfloader_device elfloader_devices[] = {
{% for d in devices %}
    {
        /* {{ d['path'] }} */
        .compat = "{{ d['compat'] }}",
        .region_bases = {
            {% for r in d['regions'] %}
            (void *){{ "0x{:x}".format(r.base) }},
            {% endfor %}
            {% for i in range(max_reg - len(d['regions'])) %}
            (void *)0,
            {% endfor %}
        },
    },
{% endfor %}
{% if devices | length == 0 %}
    {
        .compat = NULL,
        .region_bases = {
            {% for i in range(max_reg) %}
            NULL,
            {% endfor %}
        },
    },
{% endif %}
};

struct elfloader_cpu elfloader_cpus[] = {
    {% for cpu in cpus %}
    {
        /* {{ cpu['path'] }} */
        .compat = "{{ cpu['compat'] }}",
        .enable_method = {{ '"{}"'.format(cpu['enable_method']) if cpu['enable_method'] else 'NULL' }},
        .cpu_id = {{ "0x{:x}".format(cpu['cpuid']) }},
        .extra_data = {{ cpu['extra'] }}
    },
    {% endfor %}
    { .compat = NULL /* sentinel */ },
};
#else
extern struct elfloader_device elfloader_devices[];
extern struct elfloader_cpu elfloader_cpus[];
#endif
'''


def get_elfloader_cpus(tree: fdt.FdtParser, devices: list[device.WrappedNode]) -> list[dict]:
    cpus = cpu.get_cpus(tree)
    PSCI_COMPAT = ['arm,psci-0.2', 'arm,psci-1.0']
    psci_node = [n for n in devices if n.has_prop('compatible')
                 and n.get_prop('compatible').strings[0] in PSCI_COMPAT]

    if len(psci_node) > 0:
        psci_node = psci_node[0]
    else:
        psci_node = None

    cpu_info = []  # ToDo: TypeDect: str, callable? ...
    for i, cpu_node in enumerate(sorted(cpus, key=lambda a: a.path)):
        enable_method = None
        if cpu_node.has_prop('enable-method'):
            enable_method = cpu_node.get_prop('enable-method').strings[0]

        cpuid = i
        if cpu_node.has_prop('reg'):
            cpuid = cpu_node.parse_address(list(cpu_node.get_prop('reg').words))

        extra_data: Union[str, int] = 0
        if enable_method == 'psci' and psci_node:
            extra_data = 'PSCI_METHOD_' + psci_node.get_prop('method').strings[0].upper()
        elif enable_method == 'spin-table':
            extra_data = '0x{:x}'.format(
                device.Utils.make_number(2, list(cpu_node.get_prop('cpu-release-addr').words)))

        obj = {
            'compat': cpu_node.get_prop('compatible').strings[0],
            'enable_method': enable_method,
            'cpuid': cpuid,
            'path': cpu_node.path,
            'extra': extra_data,
        }
        cpu_info.append(obj)

    # guarantee that cpus in the same cluster will be consecutive
    return sorted(cpu_info, key=lambda a: a['cpuid'])


def run(tree: fdt.FdtParser, hardware: rule.HardwareYaml, config: config.Config, args: argparse.Namespace):
    devices = tree.get_elfloader_devices()
    cpu_info = get_elfloader_cpus(tree, devices)

    max_reg = 1
    device_info: list[dict] = []  # ToDo: TypeDect: str, str, List[Region]
    for dev in devices:
        regions = dev.get_regions()
        max_reg = max(len(regions), max_reg)
        obj = {
            'compat': hardware.get_matched_compatible(dev),
            'path': dev.path,
            'regions': regions
        }
        device_info.append(obj)

    device_info.sort(key=lambda a: a['compat'])

    template = Environment(loader=BaseLoader(), trim_blocks=True,
                           lstrip_blocks=True).from_string(HEADER_TEMPLATE)

    template_args = dict(builtins.__dict__, **{
        'cpus': cpu_info,
        'devices': device_info,
        'max_reg': max_reg,
        'uses_psci': any([c['enable_method'] == 'psci' for c in cpu_info])
    })

    data = template.render(template_args)
    args.elfloader_out.write(data)
    args.elfloader_out.close()


def add_args(parser: argparse.ArgumentParser):
    parser.add_argument('--elfloader-out', help='output file for elfloader header',
                        type=argparse.FileType('w'))
