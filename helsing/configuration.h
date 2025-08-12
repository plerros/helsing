// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_CONFIG_H
#define HELSING_CONFIG_H // safety precaution for the c preprocessor

#include <stdbool.h> // preprocessor true/false

// Check memory with valgrind --tool=massif

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

	#if (FANG_PAIR_OUTPUTS) && (VAMPIRE_NUMBER_OUTPUTS)
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
#define MAX_FANG_PAIRS 10
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
		#error MAX_FANG_PAIRS should be higher than MIN_FANG_PAIRS
	#endif

#define MEASURE_RUNTIME false

/*
 * ALGORITHMS:
 *
 * The included algorithms:
 * 1) normal
 * 2) cache
 *
 * They can be toggled individually.
 * When more than one algorithms are enabled, the results have to satisfy only
 * one of them.
 */

#define ALG_NORMAL false

/*
 * ALG_CACHE:
 *
 * 	This code was originally written by Jens Kruse Andersen and is included
 * with their permission. Adjustments were made to improve runtime, memory
 * usage and access patterns, and accommodate features such as multithreading.
 * Source: http://primerecords.dk/vampires/index.htm
 *
 * There are two things that this optimization does:
 *
 * 1) Reduce computations by caching & minimize cache size
 * 	Given a set of numbers {123456, 125634, 345612}, in order to convert
 * 	them to arrays of digits, 3 * 6 = 18 modulo & division operations are
 * 	required. However if we calculate and store the {12, 34, 56}, we can
 * 	reconstruct the original numbers {[12][34][56], [12][56][34],
 * 	[34][56][12]} and their arrays of digits, with only 3 * 2 = 6 modulo &
 * 	division + 3~9 load operations.
 *
 * 	Given a product A and its fangs B & C, in order to store the sum of
 * 	each digit, we can use an array of BASE elements. Because B & C are both
 * 	fangs it will be always true that arr_A and arr_B + arr_C have the same
 * 	total sum of digits and therefore we can avoid storing one of the
 * 	elements. I chose not to store the 0s.
 *
 * 	In this specific case an array of elements would waste memory due to
 * 	overhead. A more efficient solution would be to represent the array
 * 	with a single base-m number.
 *
 * 2) Data parallelism
 * 	Because each element requires very little space, we can use a single
 * 	32/64 bit unsigned integer in place of the aforementioned array.
 *
 * 	It just so happens that all the elements get processed at once (in
 * 	parallel) and memory alignment issues are avoided.
 *
 * Note to future developers; it's possible to make the array even smaller:
 * 	1. By using 32-bit elements and then expanding them to 64 bits.
 * 	   That would result in 50% array size and 247% runtime*.
 *
 * 	2. By performing extra division and modulo operations.
 * 	   That would result in 10% array size and 188% runtime*.
 *
 * 	3. By using 3 * 16-bits instead of 64-bits.
 * 	   That would result in 75% array size and 121% runtime*.
 *
 * 	*Based on some of my testing in base-10. Your mileage may vary.
 *
 * ALG_CACHE Options:
 *
 * 1) cache datatype:
 * 	The datatype used for vampire check can be found in configuration_adv.h.
 * 	Changing it will alter the performance characteristics of the program.
 * 	The space of solvable itervals will change to avoid false-positives.
 *
 * 2) PARTITION_METHOD:
 * 	Type: Semi-Constant, Semi-Global
 * 	0 - right left
 *
 * 	Type: Variable & Local
 * 	1 - left right
 * 	2 - right left
 * 	3 - left right, reserve
 * 	4 - right left, reserve
 *
 * 3) MULTIPLICAND_PARTITIONS, PRODUCT_PARTITIONS
 * 	The number will be split into this amount of partitions.
 * 	The value must be 1 or larger.
 *
 * 	A value of 1 means the number isn't split. It's processed whole like in
 * 	ALG_NORMAL. It's not recommended to use a value of 1 since it performs
 * 	poorly and uses a disproportionally large amount of memory.
 *
 * 	The default values of 2 and 3 are almost always the fastest.
 */

#define ALG_CACHE true
#define PARTITION_METHOD 0
#define MULTIPLICAND_PARTITIONS 2
#define PRODUCT_PARTITIONS 3
	#if (MULTIPLICAND_PARTITIONS <= 0)
		#error MULTIPLICAND_PARTITIONS must be larger than 0
	#endif
	#if (PRODUCT_PARTITIONS <= 0)
		#error MULTIPLICAND_PARTITIONS must be larger than 0
	#endif
	#if (MULTIPLICAND_PARTITIONS > PRODUCT_PARTITIONS)
		#warning MULTIPLICAND_PARTITIONS > PRODUCT_PARTITIONS -- performance will suffer
	#endif

/*
 * BASE:
 *
 * 	BASE defines the base of the numeral system to be used by the vampire
 * checking algorithm.
 *
 * For bases above 255 adjust digit_t accordingly.
 * If 2^(ELEMENT_BITS/(BASE-1)) < ELEMENT_BITS/log2(BASE-1), then disable ALG_CACHE.
 */

#define BASE 10
	#if (BASE < 2)
		#error BASE must be larger than 1
	#endif

/*
 * MAX_TASK_SIZE:
 *
 * Maximum value: 18446744073709551615ULL (2^64 -1)
 *
 * 	Because there is no simple way to predict the amount of vampire numbers
 * for a given interval, MAX_TASK_SIZE can be used to limit the memory usage of
 * quicksort.
 *
 * Quicksort shouldn't use more memory than:
 * threads * (sizeof(array) + MAX_TASK_SIZE * sizeof(vamp_t) * max(n_fang_pairs))
 * See https://oeis.org/A094208 for max(n_fang_pairs).
 */

#define MAX_TASK_SIZE 99999999999ULL

/*
 * USE_CHECKPOINT:
 *
 * 	USE_CHECKPOINT will generate the checkpoint file. In there the code will
 * store it's progress.
 *
 * 	The file format is text based (ASCII). The first line is like a header,
 * there we store [min] and [max], separated by a space. All the following lines
 * are optional. In those we store [complete], [count] and optionally [checksum],
 * separated by a space.
 *
 * Interfacing properly with files is hard. I have made a few design decisions
 * in the hopes to minimize the damage from possible errors in my code:
 *
 * 	1. The code always checks if checkpoint file exists before 'touch'-ing
 * 	   it. This way we prevent accidental truncation.
 *
 * 	2. The code opens the file only in read or append mode. This way we
 * 	   avoid accidental overwrite of data.
 *
 * 	3. The code has no ability to delete files. You'll have to do that
 * 	   manually.
 */

#define USE_CHECKPOINT true

/*
 * LINK_SIZE:
 *
 * The amount of elements stored in each node of an unrolled linked list.
 */

#define LINK_SIZE 100

/*
 * THREADS_PTHREAD:
 *
 * When set to false, pthreads.h won't be included. (for embedded)
 */

#define THREADS_PTHREAD true

/*
 * SAFETY_CKECKS:
 *
 * Code self check during development.
 */

#define SAFETY_CHECKS false

#endif /* HELSING_CONFIG_H */
