// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_VARGS_H
#define HELSING_VARGS_H

#include "configuration.h"
#include "llhandle.h"
#include "bthandle.h"
#include "cache.h"

struct vargs	/* Vampire arguments */
{
	vamp_t local_count;
	struct cache *digptr;

#ifdef PROCESS_RESULTS
	struct bthandle *thandle;
	struct llhandle *lhandle;
#endif
#if MEASURE_RUNTIME
	vamp_t total;
#endif
};

struct vargs *vargs_init(struct cache *digptr);
void vargs_free(struct vargs *args);
void vargs_reset(struct vargs *args);
struct llhandle *vargs_getlhandle(__attribute__((unused)) struct vargs *args);
void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax);

#ifdef PROCESS_RESULTS
void vargs_btree_cleanup(struct vargs *args, vamp_t number);
static inline void vargs_init_lhandle(struct vargs *ptr)
{
	ptr->lhandle = llhandle_init();
}
static inline void vargs_init_thandle(struct vargs *ptr)
{
	ptr->thandle = bthandle_init();
}
static inline void vargs_free_lhandle(struct vargs *ptr)
{
	free(ptr->lhandle);
}
static inline void vargs_free_thandle(struct vargs *ptr)
{
	free(ptr->thandle);
}
#else /* PROCESS_RESULTS */
static inline void vargs_btree_cleanup(
	__attribute__((unused)) struct vargs *args,
	__attribute__((unused)) vamp_t number)
{
}
static inline void vargs_init_lhandle(__attribute__((unused)) struct vargs *ptr)
{
}
static inline void vargs_init_thandle(__attribute__((unused)) struct vargs *ptr)
{
}
static inline void vargs_free_lhandle(__attribute__((unused)) struct vargs *ptr)
{
}
static inline void vargs_free_thandle(__attribute__((unused)) struct vargs *ptr)
{
}
#endif /* PROCESS_RESULTS */

#if MEASURE_RUNTIME
static inline void vargs_init_total(struct vargs *ptr)
{
	ptr->total = 0;
}
#else /* MEASURE_RUNTIME */
static inline void vargs_init_total(__attribute__((unused)) struct vargs *ptr)
{
}
#endif /* MEASURE_RUNTIME */
#endif /* HELSING_VARGS_H */