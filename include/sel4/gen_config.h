/*
 * Copyright 2024, Codasip GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

/*
 * This file exists because the kernel includes the libsel4 headers, which
 * expect to find 'sel4/gen_config.h' with the libsel4 configuration. When
 * compiling the kernel, the libsel4 configuration has not been created yet,
 * so using a dummy file is fine. Also, nothing in the libsel4 configuration
 * is relevant for the kernel.
 */
