// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026 Pierro Zachareas
 */

#ifndef HELSING_ALGORITHMS_H
#define HELSING_ALGORITHMS_H

#include <stdbool.h>

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
 * 	The datatype used for vampire check can be found in datatypes.h.
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
	#if (ALG_NORMAL && ALG_CACHE)
		#warning both ALG_NORMAL and ALG_CACHE are enabled -- performance will suffer
	#endif

#endif /* HELSING_ALGORITHMS_H */