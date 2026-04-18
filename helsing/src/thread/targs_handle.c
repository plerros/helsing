// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2026 Pierro Zachareas
 */

#include <stdlib.h>
#include <threads.h>
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

	new->read = malloc(sizeof(mtx_t));
	if (new->read == NULL)
		abort();

	mtx_init(new->read, mtx_plain);

	new->write = malloc(sizeof(mtx_t));
	if (new->write == NULL)
		abort();

	mtx_init(new->write, mtx_plain);

	new->stdout_mtx = malloc(sizeof(mtx_t));
	if (new->stdout_mtx == NULL)
		abort();

	mtx_init(new->stdout_mtx, mtx_plain);

	for (thread_t thread = 0; thread < new->options.threads; thread++) {
		new->targs[thread] = NULL;
		targs_new(&(new->targs[thread]), new->read, new->write, new->stdout_mtx, new->progress, new->digptr, new->options.dry_run);
	}
	*ptr = new;
}

void targs_handle_free(struct targs_handle *ptr)
{
	if (ptr == NULL)
		return;

	mtx_destroy(ptr->read);
	free(ptr->read);
	mtx_destroy(ptr->write);
	free(ptr->write);
	mtx_destroy(ptr->stdout_mtx);
	free(ptr->stdout_mtx);
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
		fprintf(stderr, "%u\t%.2lfs\t", thread, ptr->targs[thread]->runtime);
		helsing_fprint(stderr, "vs", ptr->targs[thread]->total, "\n");
		total_time += ptr->targs[thread]->runtime;
	}
	fprintf(stderr, "\nFang search took: %.2lf s, average: %.2lf s\n", total_time, total_time / ptr->options.threads);
#endif

	taskboard_print_results(ptr->progress);
}
