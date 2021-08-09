// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_CONFIG_H
#define HELSING_CONFIG_H // safety precaution for the c preprocessor

#include <stdbool.h> // preprocessor true/false

// Check memory with valgrind --tool=massif

#define THREADS 1

/*
 * VERBOSE_LEVEL:
 * 0 - Count fang pairs
 * 1 - Print fang pairs
 * 2 - Count vampire numbers
 * 3 - Calculate checksum
 * 4 - Print vampire numbers in OEIS format
 */

#define VERBOSE_LEVEL 2
#define DIGEST_NAME "sha512" // requires VERBOSE_LEVEL 3

#define MIN_FANG_PAIRS 1 // requires VERBOSE_LEVEL > 1

#define DISPLAY_PROGRESS false
#define MEASURE_RUNTIME false

/*
 * CACHE:
 *
 * 	This code was originally written by Jens Kruse Andersen and is included
 * with their permission. Adjustments were made to improve runtime, memory
 * usage and access patterns, and accomodate features such as multithreading.
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
 */

#define CACHE true
#define ELEMENT_BITS 64

/*
 * BASE:
 *
 *	BASE defines the base of the numerical system to be used by the vampire
 * checking algorithm.
 *
 * For bases above 255 adjust digit_t accordingly.
 */

#define BASE 10

/*
 * MAX_TASK_SIZE:
 *
 * Maximum value: 18446744073709551615ULL (2^64 -1)
 *
 * 	Because there is no simple way to predict the amount of vampire numbers
 * for a given range, MAX_TASK_SIZE can be used to limit the memory usage of
 * quicksort.
 *
 * Quicksort shouldn't use more memory than:
 * THREADS * (sizeof(array) + MAX_TASK_SIZE * sizeof(vamp_t) * max(n_fang_pairs))
 * See https://oeis.org/A094208 for max(n_fang_pairs).
 */

#define AUTO_TASK_SIZE true
#define MAX_TASK_SIZE 99999999999ULL

/*
 * USE_CHECKPOINT:
 *
 * 	USE_CHECKPOINT will generate the CHECKPOINT_FILE. In there the code will
 * store it's progress.
 *
 * 	The file format is text based (ASCII). The first line is like a header,
 * there we store [min] and [max], separated by a space. All the following lines
 * are optional. In those we store [current], [count] and optionally [checksum],
 * separated by a space.
 *
 * Interfacing properly with files is hard. I have made a few design decisions
 * in the hopes to minimize the damage from possible errors in my code:
 *
 * 	1. The code always checks if CHECKPOINT_FILE exists before 'touch'-ing
 * 	   it. This way we prevent accidental truncation.
 *
 * 	2. The code opens the file only in read or append mode. This way we
 * 	   avoid accidental overwrite of data.
 *
 * 	3. The code has no ability to delete files. You'll have to do that
 * 	   manually.
 */

#define USE_CHECKPOINT false
#define CHECKPOINT_FILE "a.checkpoint"

/*
 * LINK_SIZE:
 *
 * The amount of elements stored in each node of an unrolled linked list.
 */

#define LINK_SIZE 100
#define SANITY_CHECK false

#endif /* HELSING_CONFIG_H */