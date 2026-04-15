// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2026 Pierro Zachareas
 */

#ifndef HELSING_CONFIG_ADV_H
#define HELSING_CONFIG_ADV_H

#include <stdint.h>
#include <limits.h>
#include <time.h>
#include "configuration.h"

/*
 * Platform types configuration:
 *
 * If you modify any of the typedefs, make sure to update the corresponding max
 * value where necessary.
 */

	typedef unsigned _BitInt(BITINT_MAXWIDTH) bimax_t;
	#define BIMAX_MAX() ((unsigned _BitInt(BITINT_MAXWIDTH)) -1)

	typedef unsigned _BitInt(VAMPIRE_BITS) vamp_t; // vampire type
	#define VAMP_MAX() ((unsigned _BitInt(VAMPIRE_BITS)) -1)

	typedef unsigned _BitInt((VAMPIRE_BITS) / 2) fang_t; // fang type
	#define FANG_MAX() ((unsigned _BitInt(VAMPIRE_BITS/2)) -1)

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

	typedef unsigned _BitInt((VAMPIRE_BITS) / 2) digits_t;
	#define DIGITS_T_MAX ((unsigned _BitInt(VAMPIRE_BITS/2)) -1)

/*
 * Helper Preprocessor Macros
 */
	#if MEASURE_RUNTIME
		#if defined(CLOCK_MONOTONIC)
			#define SPDT_CLK_MODE CLOCK_MONOTONIC
		#elif defined(CLOCK_REALTIME)
			#define SPDT_CLK_MODE CLOCK_REALTIME
		#endif
	#endif

	#if (VAMPIRE_INDEX || VAMPIRE_PRINT || VAMPIRE_INTEGRAL)
		#define PRINT_RESULTS
	#endif

	#if (VAMPIRE_HASH) || (defined PRINT_RESULTS)
		#define STORE_RESULTS
	#endif

	#if VAMPIRE_HASH
		#define CHECKSUM_RESULTS
	#endif

	#define FANG_PAIRS_SIZE (MAX_FANG_PAIRS - MIN_FANG_PAIRS + 1)
	#define COUNT_ARRAY_SIZE (MAX_FANG_PAIRS + 1)

	/*
	* count[COUNT_ARRAY_REMAINDER]
	*
	* Anything that doesn't get counted as a vampire number is stored here.
	* Depending on configuration the value could store the count of vampire numbers
	* with more fang pairs than MAX_FANG_PAIRS, or all the vampire fangs.
	*/

	#define COUNT_ARRAY_REMAINDER (MAX_FANG_PAIRS)

	#if SAFETY_CHECKS
		#define OPTIONAL_ASSERT(x) assert(x)
		#include <assert.h>
	#else
		#define OPTIONAL_ASSERT(x) if(!(x)) {no_args();}
	#endif

#endif /* HELSING_CONFIG_ADV_H */
