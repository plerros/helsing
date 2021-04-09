// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_CONFIG_H
#define HELSING_CONFIG_H // safety precaution for the c preprocessor

#include <stdint.h>
#include <stdbool.h> // preprocessor true/false
#include <limits.h>
#include <time.h>

// Check memory with valgrind --tool=massif

#define THREADS 1
#define thread_t uint16_t

/*
 * VERBOSE_LEVEL:
 * 0 - Count fang pairs
 * 1 - Print fang pairs
 * 2 - Count vampire numbers
 * 3 - Print sha512 checksum
 * 4 - Print vampire numbers in OEIS format
 */

#define VERBOSE_LEVEL 2

#if VERBOSE_LEVEL >= 2
#define MIN_FANG_PAIRS 1 // requires VERBOSE_LEVEL > 1
#endif

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
 * 	each digit, we can use an array of 10 elements.	Because B & C are both
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
 * 	parallel), memory alignment issues are avoided and the base-m for a 64
 * 	bit element tops at 128	(2^7). It's as if we had assigned dedicated
 * 	bitfields and we get to use bitshift operations.
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
 * 	*Based on some of my testing. Your mileage may vary.
 */

#define CACHE true
#define ELEMENT_BITS 64

/*
 * MAX_TILE_SIZE:
 *
 * Maximum value: 18446744073709551615ULL (2^64 -1)
 *
 * 	Because there is no simple way to predict the amount of vampire numbers
 * for a given range, MAX_TILE_SIZE can be used to limit the memory usage of
 * heapsort.
 *
 * Heapsort shouldn't use more memory than:
 * THREADS * (sizeof(bthandle) + MAX_TILE_SIZE * sizeof(btree))
 */

#define AUTO_TILE_SIZE true
#define MAX_TILE_SIZE 99999999999ULL

/*
 * USE_CHECKPOINT:
 *
 * 	USE_CHECKPOINT will generate the CHECKPOINT_FILE. In there the code will
 * store it's progress every (at least) MAX_TILE_SIZE numbers.
 *
 * 	The file format is text based (ASCII). The first line is like a header,
 * there we store [min] and [max], separated by a space. All the following lines
 * are optional. In those we store [current] and [count], separated by a space.
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

#if VERBOSE_LEVEL >= 2
#define USE_CHECKPOINT false // requires VERBOSE_LEVEL > 1
#define CHECKPOINT_FILE "a.checkpoint"
#endif

/*
 * LINK_SIZE:
 *
 * 	The amount of elements stored in each link of a linked list.
 */

#define LINK_SIZE 100
#define SANITY_CHECK false

/*
 * Both vamp_t and fang_t must be unsigned, vamp_t should be double the size
 * of fang_t and fang_max, vamp_max should be set accordingly.
 *
 * You should be able to change vamp_t up to 256-bit without any issues.
 * If you want to go any higher check the uint8_t for overflow.
*/

typedef unsigned long long vamp_t;
#define vamp_max ULLONG_MAX

typedef unsigned long fang_t;
#define fang_max ULONG_MAX

typedef uint8_t digit_t;
typedef uint8_t length_t;

/*---------------------------- PREPROCESSOR STUFF ----------------------------*/
// DIGMULT = ELEMENT_BITS/(10 - DIGSKIP)
#if ELEMENT_BITS == 32
	typedef uint32_t digits_t;
	#define DIGMULT 3
	#define DIG_BASE 11
#elif ELEMENT_BITS == 64
	typedef uint64_t digits_t;
	#define DIGMULT 7
	#define DIG_BASE 128
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

#define DIGEST_NAME "sha512"

#endif /* HELSING_CONFIG_H */