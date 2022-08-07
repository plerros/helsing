// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TARGS_H
#define HELSING_TARGS_H

#include <pthread.h>
#include <stdbool.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "taskboard.h"
#include "cache.h"

#if MEASURE_RUNTIME
#include <time.h>
#endif

struct targs
{
	pthread_mutex_t *read;
	pthread_mutex_t *write;
	struct taskboard *progress;
	double	runtime;
	struct cache *digptr;
	bool dry_run;

#if MEASURE_RUNTIME
	struct timespec start;
	vamp_t total; // total amount of numbers this thread has discovered
#endif
};

void targs_new(
	struct targs **ptr,
	pthread_mutex_t *read,
	pthread_mutex_t *write,
	struct taskboard *progress,
	struct cache *digptr,
	bool dry_run);

void targs_free(struct targs *ptr);
void *thread_function(void *void_args);

#if MEASURE_RUNTIME
static inline void targs_new_total(struct targs *ptr, vamp_t total)
{
	ptr->total = total;
}
void thread_timer_start(struct targs *ptr);
void thread_timer_stop(struct targs *ptr);
#else /* MEASURE_RUNTIME */
static inline void targs_new_total(
	__attribute__((unused)) struct targs *ptr,
	__attribute__((unused)) vamp_t total)
{
}
static inline void thread_timer_start(
	__attribute__((unused)) struct targs *ptr)
{
}
static inline void thread_timer_stop(
	__attribute__((unused)) struct targs *ptr)
{
}
#endif /* MEASURE_RUNTIME */
#endif /* HELSING_TARGS_H */