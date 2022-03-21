/*
 * Copyright 2022, Axel Heider <axelheider@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <config.h>

#ifdef ENABLE_SMP_SUPPORT
#define KERNEL_STACK_ALIGNMENT 4096
#else
#define KERNEL_STACK_ALIGNMENT 8
#endif
