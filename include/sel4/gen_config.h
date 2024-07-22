/*
 * Copyright 2024, Codasip GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

/*
 * This file exists, because the kernel includes the libsel4 headers, which
 * need the header file 'sel4/gen_config.h' with the libsel4 configuration. When
 * compiling the kernel, the libsel4 configuration has not been created yet, so
 * we provide a dummy file here. This is fine because nothing in the libsel4
 * configuration is relevant for the kernel.
 */
