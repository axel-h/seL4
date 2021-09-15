/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>
#include <assert.h>
#include <stdint.h>

compile_assert(long_is_32bits, sizeof(unsigned long) == 4)

/* a "word" holds 2^5 = 32 bit */
#define wordRadix 5

typedef uint32_t timestamp_t;
