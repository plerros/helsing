// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026 Pierro Zachareas
 */

#ifndef HELSING_OTHER_H
#define HELSING_OTHER_H

#include <stdbool.h>

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
 * MAX_TASK_SIZE:
 *
 * Maximum value: 2^VAMPIRE_BITS -1
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
 * LINK_SIZE:
 *
 * The amount of elements stored in each node of an unrolled linked list.
 */

#define LINK_SIZE 100
#define LLMSENTENCE_LIMIT 10000
#define TASKBOARD_LIMIT 1000000


#define SAFETY_CHECKS false
	#if SAFETY_CHECKS
		#define OPTIONAL_ASSERT(x) assert(x)
		#include <assert.h>
	#else
		#define OPTIONAL_ASSERT(x) if(!(x)) {no_args();}
	#endif

#if (__GNUC__ || __clang__)
	#define ATTR_UNUSED      __attribute__((unused))
	#define ATTR_FALLTHROUGH __attribute__((fallthrough))
	#define ATTR_CONST       __attribute__((const))
#else
	#define ATTR_UNUSED
	#define ATTR_FALLTHROUGH
	#define ATTR_CONST
#endif

#endif /* HELSING_OTHER_H */