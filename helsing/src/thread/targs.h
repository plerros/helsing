// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TARGS_H
#define HELSING_TARGS_H

#include <pthread.h>
#include <stdbool.h> // preprocessor ifs require bool

#if MEASURE_RUNTIME
#include <time.h>
#endif

#include "configuration.h"
#include "matrix.h"
#include "cache.h"
#include <openssl/evp.h>

struct targs_t
{
	pthread_mutex_t *read;
	pthread_mutex_t *write;
	struct matrix *mat;
	vamp_t *count;
	double	runtime;
	struct cache *digptr;
	EVP_MD_CTX *mdctx;

#if MEASURE_RUNTIME
	struct timespec start;
	vamp_t total; // total amount of numbers this thread has discovered
#endif
};

struct targs_t *targs_t_init(
	pthread_mutex_t *read,
	pthread_mutex_t *write,
	struct matrix *mat,
	vamp_t *count,
	struct cache *digptr,
	EVP_MD_CTX *context);

void targs_t_free(struct targs_t *ptr);

#if MEASURE_RUNTIME
static inline void targs_init_total(
	struct targs_t *ptr,
	vamp_t total)
{
	ptr->total = total;
}
void thread_timer_start(struct targs_t *ptr);
void thread_timer_stop(struct targs_t *ptr);
#else /* MEASURE_RUNTIME */
static inline void targs_init_total(
	__attribute__((unused)) struct targs_t *ptr,
	__attribute__((unused)) vamp_t total)
{
}
static inline void thread_timer_start(
	__attribute__((unused)) struct targs_t *ptr)
{
}
static inline void thread_timer_stop(
	__attribute__((unused)) struct targs_t *ptr)
{
}
#endif /* MEASURE_RUNTIME */
#endif /* HELSING_TARGS_H */