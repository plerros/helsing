// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "taskboard.h"
#include "cache.h"
#include "targs.h"
#include "targs_handle.h"

#if SANITY_CHECK
#include <assert.h>
#endif

void targs_handle_new(struct targs_handle **ptr, vamp_t max, struct taskboard *progress)
{
#if SANITY_CHECK
	assert(ptr != NULL);
#endif

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