/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <mode/stdint.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

#define UINT64_MAX (0xFFFFFFFFFFFFFFFF)
#define UINT32_MAX (0xFFFFFFFF)
#define INT64_MAX  (0x7FFFFFFFFFFFFFFF)
#define INT32_MAX  (0x7FFFFFFF)

#define PRId64     "lld"
#define PRIi64     "lli"
#define PRIu64     "llu"
#define PRIx64     "llx"

// Check that the current compiler handles types as expected.
#define seL4_integer_check(_x_) _Static_assert(_x_, #_x_);

//
//  Data   | short | int | long | long | pointer | notes
//  model  |       |     |      | long | size_t  |
// --------+-------+-----+------+------+--------------------------
//  IP16   | 16    | 16  | 32   | 64   | 16      | MS-DOS SMALL memory model
//  LP32   | 16    | 16  | 32   | 64   | 32      | MS-DOS LARGE memory model
//  ILP32  | 16    | 32  | 32   | 64   | 32      | common for a 32-bit OS
//  LLP64  | 16    | 32  | 32   | 64   | 64      | Windows (x86-64, IA-64) and Visual C++, MinGW
//  LP64   | 16    | 32  | 64   | 64   | 64      | most Unix systems
//  ILP64  | 16    | 64  | 64   | 64   | 64      | SPARC64, Solaris
//  SILP64 | 64    | 64  | 64   | 64   | 64      | UNICOS

#if (CONFIG_WORD_SIZE == 32)

// seL4 uses ILP32 on 32-bit
seL4_integer_check( sizeof(long) == 4 );

#elif (CONFIG_WORD_SIZE == 64)

// seL4 uses LP64 64-bit systems.
seL4_integer_check( sizeof(long) == 8 );

#else
#error "unexpected value for CONFIG_WORD_SIZE"
#endif

seL4_integer_check( sizeof(int) == 4 );
seL4_integer_check( sizeof(long long) == 8 );


seL4_integer_check( sizeof(uint8_t) == 1 )
#ifndef UINT8_MAX
#define UINT8_MAX 0xffu
#endif
seL4_integer_check( (uint8_t)(-1) == UINT8_MAX )
seL4_integer_check( UINT8_MAX > 0)
seL4_integer_check( (uint8_t)(UINT8_MAX + 1) == 0)

seL4_integer_check( sizeof(int8_t) == 1 )
#ifndef INT8_MAX
#define INT8_MAX 0x7f
#endif
seL4_integer_check( (int8_t)INT8_MAX > 0 )
seL4_integer_check( (int8_t)((uint8_t)INT8_MAX + 1u) < 0 )


seL4_integer_check( sizeof(uint16_t) == 2 )
#ifndef UINT16_MAX
#define UINT16_MAX 0xffffu
#endif
seL4_integer_check( (uint16_t)(-1) == UINT16_MAX )
seL4_integer_check( (uint16_t)UINT16_MAX > 0)
seL4_integer_check( (uint16_t)(UINT16_MAX + 1) == 0)

seL4_integer_check( sizeof(int16_t) == 2 )
#ifndef INT16_MAX
#define INT16_MAX 0x7fff
#endif
seL4_integer_check( (int16_t)INT16_MAX > 0 )
seL4_integer_check( (int16_t)((uint16_t)INT16_MAX + 1u) < 0 )


seL4_integer_check( sizeof(uint32_t) == 4 )
seL4_integer_check( (uint32_t)(-1) == UINT32_MAX )
seL4_integer_check( (uint32_t)UINT32_MAX > 0)
seL4_integer_check( (uint32_t)(UINT32_MAX + 1u) == 0)

seL4_integer_check( sizeof(int32_t) == 4 )
seL4_integer_check( (int32_t)INT32_MAX > 0 )
seL4_integer_check( (int32_t)((uint32_t)INT32_MAX + 1u) < 0 )


seL4_integer_check( sizeof(uint64_t) == 8 )
seL4_integer_check( (uint64_t)(-1) == UINT64_MAX )
seL4_integer_check( (uint64_t)UINT64_MAX > 0)
seL4_integer_check( (uint64_t)(UINT64_MAX + 1u) == 0)

seL4_integer_check( sizeof(int64_t) == 8 )
seL4_integer_check( (int64_t)INT64_MAX > 0 )
seL4_integer_check( (int64_t)((uint64_t)INT64_MAX + 1u) < 0 )
