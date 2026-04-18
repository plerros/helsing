// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2026 Pierro Zachareas
 */

#ifndef HELSING_VARGS_H
#define HELSING_VARGS_H

#include <threads.h>

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
	mtx_t *stdout_mtx;
};

void vargs_new(struct vargs **ptr, struct cache *digptr, mtx_t *stdout_mtx);
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
	ATTR_UNUSED struct vargs *ptr)
{
}
#endif /* FANG_PAIR_OUTPUTS */

#if FANG_PRINT
static inline void vargs_print_results(
	mtx_t *stdout_mtx,
	vamp_t product,
	fang_t multiplier,
	fang_t multiplicand)
{
	mtx_lock(stdout_mtx);
	helsing_fprint(stdout, "vsfsfs", product, " = ", multiplier, " x ", multiplicand, "\n");
	mtx_unlock(stdout_mtx);
}
#else /* FANG_PRINT */
static inline void vargs_print_results(
	ATTR_UNUSED mtx_t *stdout_mtx,
	ATTR_UNUSED vamp_t product,
	ATTR_UNUSED fang_t multiplier,
	ATTR_UNUSED fang_t multiplicand)
{
}
#endif /* FANG_PRINT */

#if !(ALG_CACHE)
struct alg_cache
{
	int unused;
};
static inline void alg_cache_init(
	ATTR_UNUSED struct alg_cache *ptr,
	ATTR_UNUSED fang_t mod,
	ATTR_UNUSED struct cache *cache)
{
}
static inline void alg_cache_set(
	ATTR_UNUSED struct alg_cache *ptr,
	ATTR_UNUSED fang_t multiplier,
	ATTR_UNUSED fang_t multiplicand,
	ATTR_UNUSED vamp_t product,
	ATTR_UNUSED vamp_t product_iterator)
{
}
static inline void alg_cache_check(
	ATTR_UNUSED struct alg_cache *ptr,
	ATTR_UNUSED int *result)
{
}
static inline bool alg_cache_store_vamp(ATTR_UNUSED struct alg_cache *ptr)
{
	return false;
}
static inline void alg_cache_iterate_all(ATTR_UNUSED struct alg_cache *ptr)
{
}
#endif /* !ALG_CACHE */

#endif /* HELSING_VARGS_H */
