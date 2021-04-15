// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>

#include "configuration.h"
#include "cache.h"
#include "targs.h"

#if MEASURE_RUNTIME
#include <time.h>
#endif

void targs_new(
	struct targs **ptr,
	pthread_mutex_t *read,
	pthread_mutex_t *write,
	struct taskboard *progress,
	struct cache *digptr)
{
	if (ptr == NULL)
		return;

	struct targs *new = malloc(sizeof(struct targs));
	if (new == NULL)
		abort();

	new->read = read;
	new->write = write;
	new->progress = progress;
	new->runtime = 0.0;
	new->digptr = digptr;
	targs_new_total(new, 0);
	*ptr = new;
}

void targs_free(struct targs *ptr)
{
	free(ptr);
}

#if MEASURE_RUNTIME

void thread_timer_start(struct targs *ptr)
{
	clock_gettime(SPDT_CLK_MODE, &(ptr->start));
}

void thread_timer_stop(struct targs *ptr)
{
	struct timespec finish;
	clock_gettime(SPDT_CLK_MODE, &(finish));
	double elapsed = (finish.tv_sec - ptr->start.tv_sec);
	elapsed += (finish.tv_nsec - ptr->start.tv_nsec) / 1000000000.0;
	ptr->runtime = elapsed;
}

#endif /* MEASURE_RUNTIME */