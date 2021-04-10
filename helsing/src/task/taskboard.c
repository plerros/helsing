// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "configuration.h"
#include "helper.h"
#include "llhandle.h"
#include "task.h"
#include "taskboard.h"

struct taskboard *taskboard_init()
{
	struct taskboard *new = malloc(sizeof(struct taskboard));
	if (new == NULL)
		abort();

	new->tasks = NULL;
	new->size = 0;
	new->todo = 0;
	new->fmax = 0;
	taskboard_init_done(new);
	return new;
}

void taskboard_free(struct taskboard *ptr)
{
	if (ptr == NULL)
		return;

	if (ptr->tasks != NULL) {
		for (vamp_t i = 0; i < ptr->size; i++)
			task_free(ptr->tasks[i]);
		free(ptr->tasks);
	}
	free(ptr);
}

static vamp_t get_interval_size(
	__attribute__((unused)) vamp_t lmin,
	__attribute__((unused)) vamp_t lmax)
{
	vamp_t interval_size = vamp_max;

#if AUTO_TILE_SIZE
	interval_size = (lmax - lmin) / (4 * THREADS + 2);
#endif

	if (interval_size > MAX_TILE_SIZE)
		interval_size = MAX_TILE_SIZE;

	return interval_size;
}

void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax)
{
	assert(lmin <= lmax);
	assert(ptr->tasks == NULL);

	ptr->todo = 0;

	length_t fang_length = length(lmin) / 2;
	if (fang_length == length(fang_max))
		ptr->fmax = fang_max;
	else
		ptr->fmax = pow10v(fang_length); // Max factor value.

	taskboard_init_done(ptr);

	if (ptr->fmax < fang_max) {
		vamp_t fmaxsquare = ptr->fmax;
		fmaxsquare *= ptr->fmax;
		if (lmax > fmaxsquare && lmin <= fmaxsquare)
			lmax = fmaxsquare; // Max can be bigger than fmax ^ 2: 9999 > 99 ^ 2.
	}

	vamp_t interval_size = get_interval_size(lmin, lmax);

	ptr->size = div_roof((lmax - lmin + 1), interval_size + (interval_size < vamp_max));
	ptr->tasks = malloc(sizeof(struct task *) * ptr->size);
	if (ptr->tasks == NULL)
		abort();

	vamp_t x = 0;
	vamp_t iterator = interval_size;
	for (vamp_t i = lmin; i <= lmax; i += iterator + 1) {
		if (lmax - i < interval_size)
			iterator = lmax - i;

		ptr->tasks[x++] = task_init(i, i + iterator);

		if (i == lmax)
			break;
		if (i + iterator == vamp_max)
			break;
	}
	ptr->tasks[ptr->size - 1]->lmax = lmax;
}

void taskboard_reset(struct taskboard *ptr)
{
	for (vamp_t i = 0; i < ptr->size; i++)
		task_free(ptr->tasks[i]);
	free(ptr->tasks);
	ptr->tasks = NULL;
}

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)

void taskboard_print(struct taskboard *ptr, vamp_t *count)
{
	for (vamp_t x = ptr->done; x < ptr->size; x++)
		if (ptr->tasks[x] != NULL) {
			llhandle_print(ptr->tasks[x]->result, *count);
			*count += ptr->tasks[x]->result->size;
		}
}

#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */

#if defined(PROCESS_RESULTS) && DISPLAY_PROGRESS

// taskboard_progress requires mutex lock
void taskboard_progress( struct taskboard *ptr)
{
	fprintf(stderr, "%llu, %llu", ptr->tasks[ptr->done]->lmin, ptr->tasks[ptr->done]->lmax);
	fprintf(stderr, "  %llu/%llu\n", ptr->done + 1, ptr->size);
}

#endif /* defined(PROCESS_RESULTS) && DISPLAY_PROGRESS */