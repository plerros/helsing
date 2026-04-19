// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026 Pierro Zachareas
 */

#ifndef HELSING_DATATYPES_H
#define HELSING_DATATYPES_H

#include <limits.h>
#include <stdint.h>

#include "numeral_base.h"

/*
 * VAMPIRE_BITS:
 * 
 * Allows for the vampire number hunt to continue in 128-bit numbers and beyond.
 * Depends on C23's _BitInt(). Older systems will fallback to 64-bit numbers.
 */

#define VAMPIRE_BITS 64
	#ifndef BITINT_MAXWIDTH
		#undef VAMPIRE_BITS
	#endif


/*
 * Platform types configuration:
 *
 * If you modify any of the typedefs, make sure to update the corresponding max
 * value where necessary.
 */

#if defined(BITINT_MAXWIDTH)
	// If the compiler supports it, use bitint.
	typedef unsigned _BitInt(BITINT_MAXWIDTH) bimax_t;
	#define BIMAX_MAX() ((unsigned _BitInt(BITINT_MAXWIDTH)) -1)

	typedef unsigned _BitInt(VAMPIRE_BITS) vamp_t; // vampire type
	#define VAMP_MAX() ((unsigned _BitInt(VAMPIRE_BITS)) -1)

	typedef unsigned _BitInt((VAMPIRE_BITS) / 2) fang_t; // fang type
	#define FANG_MAX() ((unsigned _BitInt(VAMPIRE_BITS/2)) -1)
#else
	// Fallback values
	typedef uintmax_t bimax_t;
	#define BIMAX_MAX() UINTMAX_MAX

	typedef uint64_t vamp_t; // vampire type
	#define VAMP_MAX() UINT64_MAX

	typedef uint32_t fang_t; // fang type
	#define FANG_MAX() UINT32_MAX
#endif

typedef uint16_t thread_t;
#define THREAD_T_MAX UINT16_MAX

typedef uint8_t digit_t;
#define DIGIT_T_MAX UINT8_MAX

#if ((BASE) >= (DIGIT_T_MAX))
	#error BASE should be less than DIGIT_T_MAX
#endif

/*
 * length_t:
 * 
 * how many digits a number has in the current numeral system.
 * LENGTH_T_MAX should be greater or equal to the #bits of any type used.
 */

typedef uint16_t length_t; // I don't think we're getting 65536-bit unsigned long long anytime soon
#define LENGTH_T_MAX UINT16_MAX

/*
 * digits_t
 * 
 * Datatype of cached values in ALG_CACHE
 *
 * Affects performance, not correctness of program output. Specifically
 * ALG_CACHE is a two stage process, where stage 1 benefits from fewer
 * bits in digits_t and stage 2 benefits from more. By how much depends
 * on the args and the configuration.
 */

typedef fang_t digits_t;
#define DIGITS_T_MAX FANG_MAX()

#endif /* HELSING_DATATYPES_H */