// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <openssl/evp.h>

#include "configuration.h"
#include "helper.h"
#include "llhandle.h"
#include "task.h"
#include "taskboard.h"
#include "checkpoint.h"

struct taskboard *taskboard_init()
{
	struct taskboard *new = malloc(sizeof(struct taskboard));
	if (new == NULL)
		abort();

	new->tasks = NULL;
	new->size = 0;
	new->todo = 0;
	new->fmax = 0;
	new->done = 0;
	new->common_count = 0;

	const EVP_MD *md;
	OpenSSL_add_all_digests();

#ifdef CHECKSUM_RESULTS
	md = EVP_get_digestbyname(DIGEST_NAME);
#else
	md = EVP_md_null();
#endif

	if(!md) {
		printf("Unknown message digest %s\n", DIGEST_NAME);
		exit(1);
	}
	new->common_mdctx = EVP_MD_CTX_create();

#ifdef CHECKSUM_RESULTS
	EVP_DigestInit_ex(new->common_mdctx, md, NULL);
#endif

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
	free(ptr->common_mdctx);
	EVP_cleanup();

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

	ptr->done = 0;

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

struct task *taskboard_get_task(struct taskboard *ptr)
{
	struct task *ret = NULL;
	if (ptr->todo < ptr->size) {
		ret = ptr->tasks[ptr->todo];
		ptr->todo += 1;
	}
	return ret;
}

void taskboard_cleanup(struct taskboard *ptr)
{
	while (
		ptr->done < ptr->size &&
		ptr->tasks[ptr->done]->result != NULL)
	{
		llhandle_print(ptr->tasks[ptr->done]->result, ptr->common_count);
		llhandle_checksum(ptr->tasks[ptr->done]->result, ptr->common_mdctx);

		ptr->common_count += ptr->tasks[ptr->done]->count;
		taskboard_progress(ptr);

		save_checkpoint(ptr->tasks[ptr->done]->lmax, ptr->common_count);

		task_free(ptr->tasks[ptr->done]);
		ptr->tasks[ptr->done] = NULL;
		ptr->done += 1;
	}
}

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)

void taskboard_print(struct taskboard *ptr)
{
	for (vamp_t i = ptr->done; i < ptr->size; i++)
		task_print(ptr->tasks[i], &(ptr->common_count));
}

#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */

#if  DISPLAY_PROGRESS

// taskboard_progress requires mutex lock
void taskboard_progress( struct taskboard *ptr)
{
	fprintf(stderr, "%llu, %llu", ptr->tasks[ptr->done]->lmin, ptr->tasks[ptr->done]->lmax);
	fprintf(stderr, "  %llu/%llu\n", ptr->done + 1, ptr->size);
}

#endif /* DISPLAY_PROGRESS */