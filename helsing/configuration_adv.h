// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_CONFIG_ADV_H
#define HELSING_CONFIG_ADV_H

#include <stdint.h>
#include <limits.h>
#include <time.h>
#include "configuration.h"

/*
 * The following typedefs are used to explicate intent.
 */

typedef unsigned long long vamp_t; // vampire type
#define vamp_max ULLONG_MAX

typedef unsigned long fang_t; // fang type
#define fang_max ULONG_MAX

typedef uint16_t thread_t;
typedef uint8_t digit_t;
typedef uint8_t length_t;
#define length_t_max UINT8_MAX

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

#if (VERBOSE_LEVEL == 0)
	#define COUNT_RESULTS
#elif (VERBOSE_LEVEL == 1)
	#define DUMP_RESULTS
#elif (VERBOSE_LEVEL == 2)
	#define PROCESS_RESULTS
#elif (VERBOSE_LEVEL == 3)
	#define STORE_RESULTS
	#define PROCESS_RESULTS
	#define CHECKSUM_RESULTS
#elif (VERBOSE_LEVEL == 4)
	#define STORE_RESULTS
	#define PROCESS_RESULTS
	#define PRINT_RESULTS
#endif

#if (VERBOSE_LEVEL > 4)
#error VERBOSE_LEVEL acceptable values are 0 ~ 4
#endif

#if (ELEMENT_BITS != 32 && ELEMENT_BITS != 64)
#error ELEMENT_BITS acceptable values are 32 or 64
#endif

#if (BASE < 2)
#error BASE should be larger than 1
#endif

#endif /* HELSING_CONFIG_ADV_H */
