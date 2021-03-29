// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h> // preprocessor ifs require bool

#if MEASURE_RUNTIME
#include <time.h>
#endif

#include "configuration.h"
#include "cache.h"
#include "targs.h"

struct targs_t *targs_t_init(
	pthread_mutex_t *read,
	pthread_mutex_t *write,
	struct matrix *mat,
	vamp_t *count,
	struct cache *digptr)
{
	struct targs_t *new = malloc(sizeof(struct targs_t));
	if (new == NULL)
		abort();

	new->read = read;
	new->write = write;
	new->mat = mat;
	new->count = count;
	new->runtime = 0.0;
	new->digptr = digptr;
	targs_init_total(new, 0);
	return new;
}

void targs_t_free(struct targs_t *ptr)
{
	free(ptr);
}

#if MEASURE_RUNTIME

void thread_timer_start(struct targs_t *ptr)
{
	clock_gettime(SPDT_CLK_MODE, &(ptr->start));
}

void thread_timer_stop(struct targs_t *ptr)
{
	struct timespec finish;
	clock_gettime(SPDT_CLK_MODE, &(finish));
	double elapsed = (finish.tv_sec - ptr->start.tv_sec);
	elapsed += (finish.tv_nsec - ptr->start.tv_nsec) / 1000000000.0;
	ptr->runtime = elapsed;
}

#endif /* MEASURE_RUNTIME */