#!/usr/bin/env bash
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

while [ $# -ge 1 ]; do
    echo "#include \"$1\""
    shift
done

