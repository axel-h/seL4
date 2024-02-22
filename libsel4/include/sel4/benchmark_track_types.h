/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/config.h>
#include <stdint.h>

#if (defined CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES || defined CONFIG_DEBUG_BUILD)

/* The following IDs can be used at any point in the kernel to determine details
 * about the kernel entry point. However, there are 3 bits available to store
 * the value, thus max. 8 different values are possible.
 */
typedef enum {
    Entry_Interrupt,
    Entry_UnknownSyscall,
    Entry_UserLevelFault,
    Entry_DebugFault,
    Entry_VMFault,
    Entry_Syscall,
    Entry_UnimplementedDevice,
#if defined(CONFIG_ARCH_ARM)
    Entry_VCPUFault,
#elif defined(CONFIG_ARCH_X86)
    Entry_VMExit,
#endif
} entry_type_t;

/**
 * @brief Kernel entry logging
 *
 * Encapsulates useful info about the cause of the kernel entry
 */
typedef struct SEL4_PACKED kernel_entry {
    seL4_Word path: 3;
    union {
        struct {
            seL4_Word core: 3;
            seL4_Word word: 26;
        };
        /* Tracked kernel entry info filled from outside this file */
        struct {
            seL4_Word syscall_no: 4;
            seL4_Word cap_type: 5;
            seL4_Word is_fastpath: 1;
            seL4_Word invocation_tag: 19;
        };
    };
} kernel_entry_t;

#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES || DEBUG */

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES

typedef struct benchmark_syscall_log_entry {
    uint64_t  start_time;
    uint32_t  duration;
    kernel_entry_t entry;
} benchmark_track_kernel_entry_t;

#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES || CONFIG_DEBUG_BUILD */
