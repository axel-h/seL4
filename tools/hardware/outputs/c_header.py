#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

''' generate a c header file from the device tree '''
import argparse
import builtins
import jinja2
from typing import Dict, List, Tuple
import hardware
from hardware.config import Config
from hardware.fdt import FdtParser
from hardware.utils.rule import HardwareYaml


HEADER_TEMPLATE = '''/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
 * This file is autogenerated by <kernel>/tools/hardware/outputs/c_header.py.
 */

#pragma once

#define PHYS_BASE_RAW {{ "0x{:x}".format(physBase) }}

#ifndef __ASSEMBLER__

#include <config.h>
#include <mode/hardware.h>  /* for KDEV_BASE */
#include <linker.h>         /* for BOOT_RODATA */
#include <basic_types.h>    /* for p_region_t, kernel_frame_t (arch/types.h) */

/* Wrap raw physBase location constant to give it a symbolic name in C that's
 * visible to verification. This is necessary as there are no real constants
 * in C except enums, and enums constants must fit in an int.
 */
static inline CONST word_t physBase(void)
{
    return PHYS_BASE_RAW;
}

/* INTERRUPTS */
{% for irq in kernel_irqs %}
/* {{ irq.desc }} */
{% if irq.has_enable() %}
{{ irq.get_enable_macro_str() }}
{% endif %}
{% if irq.has_sel() %}
{{ irq.get_sel_macro_str() }}
{% endif %}
#define {{ irq.label }} {{ irq.irq }}
{% if irq.has_sel() %}
#else
#define {{ irq.label }} {{ irq.false_irq }}
{{ irq.get_sel_endif() }}
{% endif %}
{% if irq.has_enable() %}
{{ irq.get_enable_endif() }}
{% endif %}
{% endfor -%}

/* KERNEL DEVICES */
{% for (addr, macro) in sorted(kernel_macros.items()) %}
#define {{ macro }} (KDEV_BASE + {{ "0x{:x}".format(addr) }})
{% endfor %}

{% if len(kernel_regions) > 0 %}
static const kernel_frame_t BOOT_RODATA kernel_device_frames[] = {
    {% for group in kernel_regions %}
    {% if group.has_macro() %}
    {{ group.get_macro() }}
    {% endif %}
    /* {{ group.get_desc() }} */
    {% for reg in group.regions %}
    {
        .paddr = {{ "0x{:x}".format(reg.base) }},
        {% set map_addr = group.get_map_offset(reg) %}
        {% if map_addr in kernel_macros %}
        .pptr = {{ kernel_macros[map_addr] }},
        {% else %}
        /* contains {{ ', '.join(group.labels.keys()) }} */
        .pptr = KDEV_BASE + {{ "0x{:x}".format(map_addr) }},
        {% endif %}
        {% if config.arch == 'arm' %}
        .armExecuteNever = true,
        {% endif %}
        .userAvailable = {{ str(group.user_ok).lower() }}
    },
    {% endfor %}
    {% if group.has_macro() %}
    {{ group.get_endif() }}
    {% endif %}
    {% endfor %}
};

/* Elements in kernel_device_frames may be enabled in specific configurations
 * only, but the ARRAY_SIZE() macro will automatically take care of this.
 * However, one corner case remains unsolved where all elements are disabled
 * and this becomes an empty array effectively. Then the C parser used in the
 * formal verification process will fail, because it follows the strict C rules
 * which do not allow empty arrays. Luckily, we have not met this case yet...
 */
#define NUM_KERNEL_DEVICE_FRAMES ARRAY_SIZE(kernel_device_frames)
{% else %}
/* The C parser used for formal verification process follows strict C rules,
 * which do not allow empty arrays. Thus this is defined as NULL.
 */
static const kernel_frame_t BOOT_RODATA *const kernel_device_frames = NULL;
#define NUM_KERNEL_DEVICE_FRAMES 0
{% endif %}

/* PHYSICAL MEMORY */
static const p_region_t BOOT_RODATA avail_p_regs[] = {
    {% for reg in physical_memory %}
    /* {{ reg.owner.path }} */
    {
        .start = {{ "0x{:x}".format(reg.base) }},
        .end   = {{ "0x{:x}".format(reg.base + reg.size) }}
    },
    {% endfor %}
};

#endif /* !__ASSEMBLER__ */

'''


def get_kernel_devices(tree: FdtParser, hw_yaml: HardwareYaml) -> Tuple[List, Dict]:
    '''
    Given a device tree and a set of rules, returns a tuple (groups, offsets).

    Groups is a list of 'KernelRegionGroups', each of which represents a single
    contiguous region of memory that is associated with a device.
    Offsets is a dict of offset -> label, where label is the name given to the
    kernel for that address (e.g. SERIAL_PPTR) and offset is the offset from
    KDEV_BASE at which it's mapped.
    '''
    kernel_devices = tree.get_kernel_devices()

    kernel_offset = 0
    groups = []
    for dev in kernel_devices:
        dev_rule = hw_yaml.get_rule(dev)
        new_regions = dev_rule.get_regions(dev)
        for reg in new_regions:
            if reg in groups:
                other = groups[groups.index(reg)]
                other.take_labels(reg)
            else:
                groups.append(reg)

    offsets = {}
    for group in groups:
        kernel_offset = group.set_kernel_offset(kernel_offset)
        offsets.update(group.get_labelled_addresses())
    return (groups, offsets)


def get_interrupts(tree: FdtParser, hw_yaml: HardwareYaml) -> List:
    ''' Get dict of interrupts, {label: KernelInterrupt} from the DT and hardware rules. '''
    kernel_devices = tree.get_kernel_devices()

    irqs = []
    for dev in kernel_devices:
        dev_rule = hw_yaml.get_rule(dev)
        if len(dev_rule.interrupts.items()) > 0:
            irqs += dev_rule.get_interrupts(tree, dev)

    ret = {}
    for irq in irqs:
        if irq.label in ret:
            if irq.prio > ret[irq.label].prio:
                ret[irq.label] = irq
        else:
            ret[irq.label] = irq

    ret = list(ret.values())
    ret.sort(key=lambda a: a.label)
    return ret


def create_c_header_file(config, kernel_irqs: List, kernel_macros: Dict,
                         kernel_regions: List, physBase: int, physical_memory,
                         outputStream):

    jinja_env = jinja2.Environment(loader=jinja2.BaseLoader(), trim_blocks=True,
                                   lstrip_blocks=True)

    template = jinja_env.from_string(HEADER_TEMPLATE)
    template_args = dict(
        builtins.__dict__,
        **{
            'config': config,
            'kernel_irqs': kernel_irqs,
            'kernel_macros': kernel_macros,
            'kernel_regions': kernel_regions,
            'physBase': physBase,
            'physical_memory': physical_memory})
    data = template.render(template_args)

    with outputStream:
        outputStream.write(data)


def run(tree: FdtParser, hw_yaml: HardwareYaml, config: Config, args: argparse.Namespace):
    if not args.header_out:
        raise ValueError('You need to specify a header-out to use c header output')

    physical_memory, reserved, physBase = hardware.utils.memory.get_physical_memory(tree, config)
    kernel_regions, kernel_macros = get_kernel_devices(tree, hw_yaml)

    create_c_header_file(
        config,
        get_interrupts(tree, hw_yaml),
        kernel_macros,
        kernel_regions,
        physBase,
        physical_memory,
        args.header_out)


def add_args(parser):
    parser.add_argument('--header-out', help='output file for c header',
                        type=argparse.FileType('w'))
