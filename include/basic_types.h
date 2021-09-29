/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stdint.h>
#include <arch/types.h>

/* arch/types.h is supposed to define word_t and _seL4_word_fmt */
#ifndef _seL4_word_fmt
#error "missing _seL4_word_fmt"
#endif

/* Using multiple macro layers may look strange, but this is required to make
 * the preprocessor fully evaluate all macro parameters first and then pass the
 * result as parameter to the next macro layer. This allows passing macros as
 * parameters also, and not just plain strings. The final concatenation will
 * always be from the strings behind all macros then - and not the macro names
 * that are passed as parameters.
 */
#define _macro_concat_helper2(x,y,z)    x ## y ## z
#define _macro_concat_helper(x,y,z)     _macro_concat_helper2(x,y,z)

#define _macro_str_concat_helper2(x)    #x
#define _macro_str_concat_helper1(x,y)  _macro_str_concat_helper2(x ## y)
#define _macro_str_concat(x,y)          _macro_str_concat_helper1(x,y)

#define SEL4_PRIu_word  _macro_str_concat(_seL4_word_fmt, u)
#define SEL4_PRIx_word  _macro_str_concat(_seL4_word_fmt, x)
#define SEL4_PRI_word   SEL4_PRIu_word

/* The C parser from the verification toolchain requires declaring word_t
 * constants without casting integer values to word_t. Since the printf() format
 * specifiers are aligned with the C integer type suffixes, _seL4_word_fmt can
 * be used there also.
 */
#define SEL4_WORD_CONST(x)  _macro_concat_helper(x, _seL4_word_fmt, u)

enum _bool {
    false = 0,
    true  = 1
};
typedef word_t bool_t;

/* equivalent to a word_t except that we tell the compiler that we may alias with
 * any other type (similar to a char pointer) */
typedef word_t __attribute__((__may_alias__)) word_t_may_alias;
