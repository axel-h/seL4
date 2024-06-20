/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

/* includes by convention, regardless of a dependency */
#include <config.h>
/* includes due to an actual dependency */
#include <stdint.h>
#include <assert.h>

compile_assert(long_is_32bits, sizeof(unsigned long) == 4)

typedef uint32_t timestamp_t;
