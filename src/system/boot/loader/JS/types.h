
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Gil Mendes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 * @brief		Type definitions.
 */

#ifndef __TYPES_H
#define __TYPES_H

#include "compiler.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __LP64__
# define __INT_64	long
# define __PRI_64	"l"
#else
# define __INT_64	long long
# define __PRI_64	"ll"
#endif

// Format character definitions for printf().
#define J_PRIu8		"u"				// Format for uint8_t.
#define J_PRIu16		"u"				// Format for uint16_t.
#define J_PRIu32		"u"				// Format for uint32_t.
#define J_PRIu64		__PRI_64 "u"	// Format for uint64_t.
#define J_PRId8		"d"				// Format for int8_t.
#define J_PRId16		"d"				// Format for int16_t.
#define J_PRId32		"d"				// Format for int32_t.
#define J_PRId64		__PRI_64 "d"	// Format for int64_t.
#define J_PRIx8		"x"				// Format for (u)int8_t (hexadecimal).
#define J_PRIx16		"x"				// Format for (u)int16_t (hexadecimal).
#define J_PRIx32		"x"				// Format for (u)int32_t (hexadecimal).
#define J_PRIx64		__PRI_64 "x"	// Format for (u)int64_t (hexadecimal).
#define J_PRIo8		"o"				// Format for (u)int8_t (octal).
#define J_PRIo16		"o"				// Format for (u)int16_t (octal).
#define J_PRIo32		"o"				// Format for (u)int32_t (octal).
#define J_PRIo64		__PRI_64 "o"	// Format for (u)int64_t (octal).

// Unsigned data types.
typedef unsigned char uint8_t;		// Unsigned 8-bit.
typedef unsigned short uint16_t;	// Unsigned 16-bit.
typedef unsigned int uint32_t;		// Unsigned 32-bit.
typedef unsigned __INT_64 uint64_t;	// Unsigned 64-bit.

// Signed data types.
typedef signed char int8_t;			// Signed 8-bit.
typedef signed short int16_t;		// Signed 16-bit.
typedef signed int int32_t;			// Signed 32-bit.
typedef signed __INT_64 int64_t;	// Signed 64-bit.

// Extra integer types.
typedef uint64_t offset_t;		// Type used to store an offset into a file/device.
typedef int64_t mstime_t;		// Type used to store a time value in milliseconds.

// Type limit macros.
#define J_INT8_MIN	(-128)
#define J_INT8_MAX	127
#define J_UINT8_MAX	255u
#define J_INT16_MIN	(-32767-1)
#define J_INT16_MAX	32767
#define J_UINT16_MAX	65535u
#define J_INT32_MIN	(-2147483647-1)
#define J_INT32_MAX	2147483647
#define J_UINT32_MAX	4294967295u
#define J_INT64_MIN	(-9223372036854775807ll-1)
#define J_INT64_MAX	9223372036854775807ll
#define J_UINT64_MAX	18446744073709551615ull
#ifdef __LP64__
#   define J_SIZE_MAX     J_UINT64_MAX
#else
#   define J_SIZE_MAX     J_UINT32_MAX
#endif

// Number of bits in a char.
#define J_CHAR_BIT	8

#include "arch/types.h"

#endif /* __TYPES_H */
