// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_VARGS_H
#define HELSING_VARGS_H

#include "configuration_adv.h"
#include "cache.h"
#include "array.h"

#if FANG_PRINT
#include <stdio.h>
#endif

struct vargs /* Vampire arguments */
{
	struct cache *digptr;
	struct array *result;
	vamp_t local_count[COUNT_ARRAY_SIZE];
};

void vargs_new(struct vargs **ptr, struct cache *digptr);
void vargs_free(struct vargs *args);
void vargs_reset(struct vargs *args);
void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax);

#if FANG_PAIR_OUTPUTS
static inline void vargs_iterate_local_count(struct vargs *ptr)
{
	ptr->local_count[COUNT_ARRAY_REMAINDER] += 1;
}
#else /* FANG_PAIR_OUTPUTS */
static inline void vargs_iterate_local_count(
	__attribute__((unused)) struct vargs *ptr)
{
}
#endif /* FANG_PAIR_OUTPUTS */

#if FANG_PRINT
static inline void vargs_print_results(
	vamp_t product,
	fang_t multiplier,
	fang_t multiplicand)
{
	flockfile(stdout);
	printf("%ju = %ju x %ju\n",(uintmax_t)product, (uintmax_t)multiplier, (uintmax_t)multiplicand);
	funlockfile(stdout);
}
#else /* FANG_PRINT */
static inline void vargs_print_results(
	__attribute__((unused)) vamp_t product,
	__attribute__((unused)) fang_t multiplier,
	__attribute__((unused)) fang_t multiplicand)
{
}
#endif /* FANG_PRINT */

#if !(ALG_NORMAL)
static inline void alg_normal_set(
	__attribute__((unused)) fang_t multiplier,
	__attribute__((unused)) length_t (*mult_array)[BASE])
{
}
static inline void alg_normal_check(
	__attribute__((unused)) length_t mult_array[BASE],
	__attribute__((unused)) fang_t multiplicand,
	__attribute__((unused)) vamp_t product,
	__attribute__((unused)) int *result)
{
}
#endif /* !ALG_NORMAL */

#if !(ALG_CACHE)
struct alg_cache
{
};
static inline void alg_cache_init(
	__attribute__((unused)) struct alg_cache *ptr,
	__attribute__((unused)) fang_t mod,
	__attribute__((unused)) struct cache *cache)
{
}
static inline void alg_cache_set(
	__attribute__((unused)) struct alg_cache *ptr,
	__attribute__((unused)) fang_t multiplier,
	__attribute__((unused)) fang_t multiplicand,
	__attribute__((unused)) vamp_t product,
	__attribute__((unused)) vamp_t product_iterator)
{
}
static inline void alg_cache_check(
	__attribute__((unused)) struct alg_cache *ptr,
	__attribute__((unused)) int *result)
{
}
static inline void alg_cache_iterate_all(__attribute__((unused)) struct alg_cache *ptr)
{
}
#endif /* !ALG_CACHE */

#endif /* HELSING_VARGS_H */
