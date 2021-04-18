// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>

#include "configuration.h"
#include "cache.h"
#include "targs.h"
#include "vargs.h"

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

void *thread_function(void *void_args)
{
	struct targs *args = (struct targs *)void_args;
	thread_timer_start(args);
	struct vargs *vamp_args;
	vargs_new(&(vamp_args), args->digptr);
	struct task *current = NULL;

	do {
		current = NULL;
// Critical section start
		pthread_mutex_lock(args->read);

		current = taskboard_get_task(args->progress);

		pthread_mutex_unlock(args->read);
// Critical section end

		if (current != NULL) {
			vampire(current->lmin, current->lmax, vamp_args, args->progress->fmax);

// Critical section start
			pthread_mutex_lock(args->write);

			task_copy_vargs(current, vamp_args);
#if MEASURE_RUNTIME
			args->total += current->count;
#endif
			taskboard_cleanup(args->progress);

			pthread_mutex_unlock(args->write);

// Critical section end
			vargs_reset(vamp_args);
		}
	} while (current != NULL);
	vargs_free(vamp_args);
	thread_timer_stop(args);
	return 0;
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