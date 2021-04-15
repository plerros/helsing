// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "configuration.h"
#include "llhandle.h"
#include "taskboard.h"
#include "cache.h"
#include "vargs.h"
#include "targs.h"
#include "targs_handle.h"

void targs_handle_new(struct targs_handle **ptr, vamp_t max, struct taskboard *progress)
{
	if (ptr == NULL)
		return;

	struct targs_handle *new = malloc(sizeof(struct targs_handle));
	if (new == NULL)
		abort();

	new->progress = progress;
	cache_new(&(new->digptr), max);

	new->read = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->read, NULL);
	new->write = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->write, NULL);

	for (thread_t thread = 0; thread < THREADS; thread++)
		targs_new(&(new->targs[thread]), new->read, new->write, new->progress, new->digptr);
	*ptr = new;
}

void targs_handle_free(struct targs_handle *ptr)
{
	if (ptr == NULL)
		return;

	pthread_mutex_destroy(ptr->read);
	free(ptr->read);
	pthread_mutex_destroy(ptr->write);
	free(ptr->write);
	taskboard_free(ptr->progress);
	cache_free(ptr->digptr);

	for (thread_t thread = 0; thread < THREADS; thread++)
		targs_free(ptr->targs[thread]);

	free(ptr);
}

void targs_handle_print(struct targs_handle *ptr)
{
#if MEASURE_RUNTIME
	double total_time = 0.0;
	fprintf(stderr, "Thread  Runtime Count\n");
	for (thread_t thread = 0; thread<THREADS; thread++) {
		fprintf(stderr, "%u\t%.2lfs\t%llu\n", thread, ptr->targs[thread]->runtime, ptr->targs[thread]->total);
		total_time += ptr->targs[thread]->runtime;
	}
	fprintf(stderr, "\nFang search took: %.2lf s, average: %.2lf s\n", total_time, total_time / THREADS);
#endif

	taskboard_print_results(ptr->progress);
}

void *thread_worker(void *void_args)
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
