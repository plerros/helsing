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

#define ELEMENT_BITS COMPARISON_BITS
#define ACTIVE_BITS ELEMENT_BITS

/*
 * The following typedefs are used to explicate intent.
 */

typedef unsigned long long vamp_t; // vampire type
#define VAMP_MAX ULLONG_MAX

typedef unsigned long fang_t; // fang type
#define FANG_MAX ULONG_MAX

typedef uint16_t thread_t;
#define THREAD_MAX UINT16_MAX
typedef uint8_t digit_t;
typedef uint8_t length_t;
#define LENGTH_T_MAX UINT8_MAX

#if ELEMENT_BITS == 32
	typedef uint32_t digits_t;
#elif ELEMENT_BITS == 64
	typedef uint64_t digits_t;
#endif

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

#define FANG_ARRAY_SIZE  (MAX_FANG_PAIRS)
#define COUNT_ARRAY_SIZE (MAX_FANG_PAIRS + 1)
#define COUNT_ARRAY_REMAINDER (MAX_FANG_PAIRS)

#if (FANG_PAIR_OUTPUTS) && (VAMPIRE_NUMBER_OUTPUTS)
#warning Both FANG_PAIR_OUTPUTS and VAMPIRE_NUMBER_OUTPUTS are true
#endif

#if (MIN_FANG_PAIRS == 0)
#error MIN_FANG_PAIRS must be larger than 0
#endif

#if (MIN_FANG_PAIRS > 1 && !(VAMPIRE_NUMBER_OUTPUTS))
#error MIN_FANG_PAIRS > 1 requires VAMPIRE_NUMBER_OUTPUTS
#endif

#if (MAX_FANG_PAIRS == 0)
#error MAX_FANG_PAIRS must be larger than 0
#endif

#if (MAX_FANG_PAIRS > 1 && !(VAMPIRE_NUMBER_OUTPUTS))
#error MAX_FANG_PAIRS > 1 requires VAMPIRE_NUMBER_OUTPUTS
#endif

#if (MAX_FANG_PAIRS < MIN_FANG_PAIRS)
#error MAX_FANG_PAIRS should be higher than MIN_FANG_PAIRS
#endif

#if (COMPARISON_BITS != 32 && COMPARISON_BITS != 64)
#error COMPARISON_BITS acceptable values are 32 or 64
#endif

#if (MULTIPLICAND_PARTITIONS == 0)
	#error MULTIPLICAND_PARTITIONS must be larger than 0
#endif

#if (PRODUCT_PARTITIONS == 0)
	#error MULTIPLICAND_PARTITIONS must be larger than 0
#endif

#if (MULTIPLICAND_PARTITIONS > PRODUCT_PARTITIONS)
	//#warning MULTIPLICAND_PARTITIONS > PRODUCT_PARTITIONS -- performance will suffer
#endif

#if (BASE < 2)
#error BASE must be larger than 1
#endif

#if SAFETY_CHECKS
	#define OPTIONAL_ASSERT(x) assert(x)
	#include <assert.h>
#else
	#define OPTIONAL_ASSERT(x) if(!(x)) {no_args();}
#endif

#endif /* HELSING_CONFIG_ADV_H */
