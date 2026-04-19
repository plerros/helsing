// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026 Pierro Zachareas
 */

#ifndef HELSING_OUTPUT_H
#define HELSING_OUTPUT_H

#include <stdbool.h>
#include <time.h>

/*
 * FANG_PAIR_OUTPUTS:
 *
 * FANG_PRINT:
 * 	print fang pairs
 */

#define FANG_PAIR_OUTPUTS false
#define FANG_PRINT false

/*
 * VAMPIRE_NUMBER_OUTPUTS:
 *
 * VAMPIRE_INDEX
 * VAMPIRE_PRINT
 * VAMPIRE_INTEGRAL
 *
 * VAMPIRE_HASH
 *
 * OEIS is VAMPIRE_INDEX and VAMPIRE_PRINT
 */

#define VAMPIRE_NUMBER_OUTPUTS true
#define VAMPIRE_INDEX    false
#define VAMPIRE_PRINT    false
#define VAMPIRE_INTEGRAL false
#define VAMPIRE_HASH     false

#define DIGEST_NAME "sha512" // requires VERBOSE_LEVEL 3
	#if (FANG_PRINT) && ((VAMPIRE_INDEX) || (VAMPIRE_PRINT) || (VAMPIRE_INTEGRAL) || (VAMPIRE_HASH))
		#warning Both FANG_PAIR_OUTPUTS and VAMPIRE_NUMBER_OUTPUTS are true. Result streams will be interleaved.
	#endif


/*
 * MIN_FANG_PAIRS:
 *
 * 	Filter out vampire numbers whose fang pair count is < MIN_FANG_PAIRS.
 * Requires VERBOSE_LEVEL > 1.
 *
 * MAX_FANG_PAIRS:
 *
 * 	For each n within the [MIN_FANG_PAIRS, MAX_FANG_PAIRS], print results
 * separately. Also requires VERBOSE_LEVEL > 1.
 *
 * When VERBOSE_LEVEL=4 we have n lists to print out. 1 for each possible value
 * within the [MIN_FANG_PAIRS, MAX_FANG_PAIRS] range. However we only have 1
 * file stream for output.
 *
 * It would't be possible with the current code architecture to store the
 * results and print the lists consecutively:
 * 	A(1)
 * 	A(2)
 * 	...
 * 	A(N)
 *
 * 	B(1)
 * 	B(2)
 * 	...
 * 	B(N)
 *
 * As a compromise the lists are interleaved as such:
 * 	A(1)
 * 	A(2)
 * 	...
 * 	A(k)
 * 		B(1)
 * 	A(k+1)
 * 	...
 * 		B(2)
 * 	...
 * 		...
 * 	A(N)
 *
 * Elements from the n-th list have n tabs before them.
 */

#define MIN_FANG_PAIRS 1
#define MAX_FANG_PAIRS 1
	#if (MIN_FANG_PAIRS == 0)
		#error MIN_FANG_PAIRS must be larger than 0
	#endif
	#if (MAX_FANG_PAIRS == 0)
		#error MAX_FANG_PAIRS must be larger than 0
	#endif
	#if (MIN_FANG_PAIRS > 1 && !(VAMPIRE_NUMBER_OUTPUTS))
		#error MIN_FANG_PAIRS > 1 requires VAMPIRE_NUMBER_OUTPUTS
	#endif
	#if (MAX_FANG_PAIRS > 1 && !(VAMPIRE_NUMBER_OUTPUTS))
		#error MAX_FANG_PAIRS > 1 requires VAMPIRE_NUMBER_OUTPUTS
	#endif
	#if (MAX_FANG_PAIRS < MIN_FANG_PAIRS)
		#error MAX_FANG_PAIRS should be bigger than MIN_FANG_PAIRS
	#endif


#define MEASURE_RUNTIME false


// advanced

	#if (VAMPIRE_INDEX || VAMPIRE_PRINT || VAMPIRE_INTEGRAL)
		#define PRINT_RESULTS
	#endif
	#if (VAMPIRE_HASH) || (defined PRINT_RESULTS)
		#define STORE_RESULTS
	#endif
	#if VAMPIRE_HASH
		#define CHECKSUM_RESULTS
	#endif

	#if MEASURE_RUNTIME
		#if defined(CLOCK_MONOTONIC)
			#define SPDT_CLK_MODE CLOCK_MONOTONIC
		#elif defined(CLOCK_REALTIME)
			#define SPDT_CLK_MODE CLOCK_REALTIME
		#endif
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

#endif /* HELSING_OUTPUT_H */