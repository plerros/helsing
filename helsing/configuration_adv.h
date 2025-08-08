// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
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
 * These are good defaults for 64-bit systems.
 *
 * You may be able to improve performance in 32-bit systems by adjusting some
 * of these down to 32-bit types, or increase the solvable space by adjusting
 * them up (128-bit, ...)
 * 
 * If you modify any of the typedefs, make sure to update the corresponding max
 * value. The code assumes you've done that correctly.
 */

	typedef unsigned long long vamp_t; // vampire type
	#define VAMP_MAX ULLONG_MAX
	//typedef __uint128_t vamp_t;      // vampire type
	//static const __uint128_t UINT128_MAX =__uint128_t(__int128_t(-1L));
	//#define VAMP_MAX UINT128_MAX

	typedef unsigned long fang_t; // fang type
	#define FANG_MAX ULONG_MAX
		#if (FANG_MAX > VAMP_MAX)
			#error "VAMP_MAX should be >= than FANG_MAX"
		#endif

	typedef uint16_t thread_t;
	#define THREAD_T_MAX UINT16_MAX

	typedef uint8_t digit_t;
	#define DIGIT_T_MAX BASE

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
	 * Datatype of the cache used in ALG_CACHE
	 * 
	 * On 32-bit systems this can be set to:
	 * 	typedef uint_least32_t digits_t;
	 * 	#define DIGITS_T_MAX UINT_LEAST32_MAX
	 * The performance will improve at the cost of not being able to solve bigger problems.
	 */

	typedef uint_fast64_t digits_t;
	#define DIGITS_T_MAX UINT_FAST64_MAX

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
