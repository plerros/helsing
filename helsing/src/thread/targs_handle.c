// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdio.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"
#include "taskboard.h"
#include "cache.h"
#include "targs.h"
#include "targs_handle.h"

void targs_handle_new(struct targs_handle **ptr, struct options_t options, vamp_t min, vamp_t max, struct taskboard *progress)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct targs_handle *new = malloc(sizeof(struct targs_handle));
	if (new == NULL)
		abort();

	new->options = options;
	new->progress = progress;
	new->digptr = NULL;
	cache_new(&(new->digptr), min, max);

	new->targs = malloc(sizeof(struct targs *) * new->options.threads);
	if (new->targs == NULL)
		abort();

	new->read = NULL;
	mutex_new(&(new->read));
	new->write = NULL;
	mutex_new(&(new->write));

	for (thread_t thread = 0; thread < new->options.threads; thread++) {
		new->targs[thread] = NULL;
		targs_new(&(new->targs[thread]), new->read, new->write, new->progress, new->digptr, new->options.dry_run);
	}
	*ptr = new;
}

void targs_handle_free(struct targs_handle *ptr)
{
	if (ptr == NULL)
		return;

	mutex_free(ptr->read);
	mutex_free(ptr->write);
	cache_free(ptr->digptr);

	for (thread_t thread = 0; thread < ptr->options.threads; thread++)
		targs_free(ptr->targs[thread]);

	free(ptr->targs);
	free(ptr);
}

void targs_handle_print(struct targs_handle *ptr)
{
#if MEASURE_RUNTIME
	double total_time = 0.0;
	fprintf(stderr, "Thread  Runtime Count\n");
	for (thread_t thread = 0; thread < ptr->options.threads; thread++) {
		fprintf(stderr, "%u\t%.2lfs\t%ju\n", thread, ptr->targs[thread]->runtime, (uintmax_t)(ptr->targs[thread]->total));
		total_time += ptr->targs[thread]->runtime;
	}
	fprintf(stderr, "\nFang search took: %.2lf s, average: %.2lf s\n", total_time, total_time / ptr->options.threads);
#endif

	taskboard_print_results(ptr->progress);
}
