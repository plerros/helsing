// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2027 Pierro Zachareas
 */

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"
#include "array.h"
#include "task.h"
#include "taskboard.h"
#include "checkpoint.h"
#include "hash.h"

void taskboard_new(struct taskboard **ptr, struct options_t options)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct taskboard *new = malloc(sizeof(struct taskboard));
	if (new == NULL)
		abort();

	new->options = options;
	new->tasks = NULL;
	new->size = 0;
	new->todo = 0;
	new->fmax = 0;
	new->done = 0;
	memset(new->common_count, 0, sizeof(new->common_count));
	memset(new->common_prev, 0, sizeof(new->common_prev));
	new->checksum = NULL;
	hash_new(&(new->checksum));
	*ptr = new;
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
	hash_free(ptr->checksum);
	free(ptr);
}

static vamp_t get_interval_size(struct options_t options, vamp_t lmin, vamp_t lmax)
{
	vamp_t interval_size = VAMP_MAX;

	if (options.manual_task_size != 0) {
		interval_size = options.manual_task_size;
	} else {
		interval_size = (lmax - lmin) / (4 * options.threads + 2);

		if (interval_size > MAX_TASK_SIZE)
			interval_size = MAX_TASK_SIZE;
	}

	return interval_size;
}

static inline void taskboard_set_task(struct taskboard *ptr, size_t index)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(index >= ptr->todo);

	vamp_t l_bound = ptr->lmin + (ptr->interval_size + 1) * index;
	vamp_t u_bound = l_bound;
	if (VAMP_MAX - ptr->interval_size > u_bound)
		u_bound += ptr->interval_size ;
	if (u_bound > ptr->lmax)
		u_bound = ptr->lmax;

	task_new(&(ptr->tasks[index]), l_bound, u_bound);
}

void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax)
{
	assert(ptr->done == ptr->size);
	for (vamp_t i = 0; i < ptr->size; i++)
		task_free(ptr->tasks[i]);
	free(ptr->tasks);
	ptr->tasks = NULL;
	ptr->size = 0;
	ptr->todo = 0;
	ptr->done = 0;
	ptr->fmax = 0;

	assert(lmin <= lmax);

	length_t fang_length = length(lmin) / 2;
	if (fang_length == length(FANG_MAX))
		ptr->fmax = FANG_MAX;
	else if (fang_length == 0)
		ptr->fmax = 0;
	else
		ptr->fmax = pow_v(fang_length) - 1; // Max factor value.

	/*
	 * Adjust lmax -- we can skip values bigger than fmax^2:
	 * BASE^(2n) - 1 > (BASE^n - 1) ^ 2
	 */
	if (
		(ptr->fmax > 0) // Div zero guard
		&& (ptr->fmax < VAMP_MAX / ptr->fmax) // Overflow guard
	) {
		vamp_t fmaxsquare = ptr->fmax;
		fmaxsquare *= ptr->fmax;
		if (fmaxsquare < lmin)
			return;
		else if (fmaxsquare < lmax)
			lmax = fmaxsquare;
	}

	ptr->lmin = lmin;
	ptr->lmax = lmax;
	ptr->interval_size = get_interval_size(ptr->options, lmin, lmax);

	ptr->size = div_roof((lmax - lmin + 1), ptr->interval_size + (ptr->interval_size < VAMP_MAX));

	ptr->tasks = malloc(sizeof(struct task *) * ptr->size);
	if (ptr->tasks == NULL)
		abort();

	for (vamp_t i = 0; i < ptr->size; i++)
		ptr->tasks[i] = NULL;

	for (vamp_t i = 0; i < ptr->size && i < TASKBOARD_LIMIT; i++)
		taskboard_set_task(ptr, i);	
}

struct task *taskboard_get_task(struct taskboard *ptr)
{
	if (ptr->todo >= ptr->size)
		return NULL;

	size_t todo = ptr->todo;

	if (ptr->tasks[ptr->todo] == NULL) {
		size_t interval = ptr->size - ptr->todo;
		if (interval > TASKBOARD_LIMIT)
			interval = TASKBOARD_LIMIT;

		for (size_t i = 0; i < interval; i++) {
			taskboard_set_task(ptr, i + ptr->todo);
		}

	}
	struct task *ret = ptr->tasks[todo];
	ptr->todo += 1;
	return ret;
}

void taskboard_cleanup(struct taskboard *ptr)
{
	while (
		ptr->done < ptr->size &&
		ptr->tasks[ptr->done] != NULL &&
		ptr->tasks[ptr->done]->complete != false)
	{
		if (ptr->tasks[ptr->done]->result != NULL) {
			array_print(ptr->tasks[ptr->done]->result, ptr->common_count, &(ptr->common_prev));
			array_checksum(ptr->tasks[ptr->done]->result, ptr->checksum);
		}
		for (size_t i = 0; i < COUNT_ARRAY_SIZE; i++)
			ptr->common_count[i] += ptr->tasks[ptr->done]->count[i];
		taskboard_progress(ptr);
		if (!ptr->options.dry_run)
			save_checkpoint(ptr->options, ptr->tasks[ptr->done]->lmax, ptr);

		task_free(ptr->tasks[ptr->done]);
		ptr->tasks[ptr->done] = NULL;
		ptr->done += 1;
	}
}

void taskboard_print_results(struct taskboard *ptr)
{
	#if FANG_PAIR_OUTPUTS
		vamp_t sum = 0;
		for (size_t i = 0; i < COUNT_ARRAY_SIZE; i++)
			sum += ptr->common_count[i];
		fprintf(stderr, "Found: %ju fang pair(s).\n", (uintmax_t)sum);
	#endif

	#if VAMPIRE_NUMBER_OUTPUTS
		fprintf(stderr, "Found: %ju vampire number(s).\n", (uintmax_t)(ptr->common_count[MIN_FANG_PAIRS - 1]));
	#endif
	for (size_t i = MIN_FANG_PAIRS; i < MAX_FANG_PAIRS; i++) {
		if (ptr->common_count[i] == 0)
			continue;

		if (i == MIN_FANG_PAIRS)
			fprintf(stderr, "Out of which:\n");


		fprintf(stderr, "\t%ju\thave at least %zu fang pair(s)\n", (uintmax_t)(ptr->common_count[i]), i+1);
	}
	hash_print(ptr->checksum);
}

// taskboard_progress requires mutex lock
void taskboard_progress(struct taskboard *ptr)
{
	if (ptr->options.display_progress) {
		fprintf(stderr, "%ju, %ju", (uintmax_t)(ptr->tasks[ptr->done]->lmin), (uintmax_t)(ptr->tasks[ptr->done]->lmax));
		fprintf(stderr, "  %ju/%ju\n", (uintmax_t)(ptr->done + 1), (uintmax_t)(ptr->size));
	}
}
