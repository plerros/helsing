// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_VARGS_H
#define HELSING_VARGS_H

#include "configuration_adv.h"
#include "cache.h"
#include "array.h"

#ifdef DUMP_RESULTS
#include <stdio.h>
#endif

struct vargs /* Vampire arguments */
{
	struct cache *digptr;
	struct array *result;
	vamp_t local_count;
};

void vargs_new(struct vargs **ptr, struct cache *digptr);
void vargs_free(struct vargs *args);
void vargs_reset(struct vargs *args);
void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax);

#if defined COUNT_RESULTS || defined DUMP_RESULTS
static inline void vargs_iterate_local_count(struct vargs *ptr)
{
	ptr->local_count += 1;
}
#else /* defined COUNT_RESULTS || defined DUMP_RESULTS */
static inline void vargs_iterate_local_count(
	__attribute__((unused)) struct vargs *ptr)
{
}
#endif /* defined COUNT_RESULTS || defined DUMP_RESULTS */

#ifdef DUMP_RESULTS
static inline void vargs_print_results(
	vamp_t product,
	fang_t multiplier,
	fang_t multiplicand)
{
	flockfile(stdout);
	printf("%llu = %lu x %lu\n", product, multiplier, multiplicand);
	funlockfile(stdout);
}
#else /* DUMP_RESULTS */
static inline void vargs_print_results(
	__attribute__((unused)) vamp_t product,
	__attribute__((unused)) fang_t multiplier,
	__attribute__((unused)) fang_t multiplicand)
{
}
#endif /* DUMP_RESULTS */
#endif /* HELSING_VARGS_H */