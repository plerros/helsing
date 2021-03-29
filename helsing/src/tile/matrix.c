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
#include "tile.h"
#include "matrix.h"

struct matrix *matrix_init()
{
	struct matrix *new = malloc(sizeof(struct matrix));
	if (new == NULL)
		abort();

	new->arr = NULL;
	new->size = 0;
	new->unfinished = 0;
	new->fmax = 0;
	matrix_init_cleanup(new);
	return new;
}

void matrix_free(struct matrix *ptr)
{
	if (ptr == NULL)
		return;

	if (ptr->arr != NULL) {
		for (vamp_t i = 0; i < ptr->size; i++)
			tile_free(ptr->arr[i]);
		free(ptr->arr);
	}
	free(ptr);
}

static vamp_t get_tilesize(
	__attribute__((unused)) vamp_t lmin,
	__attribute__((unused)) vamp_t lmax)
{
	vamp_t tile_size = vamp_max;

#if AUTO_TILE_SIZE
	tile_size = (lmax - lmin) / (4 * THREADS + 2);
#endif

	if (tile_size > MAX_TILE_SIZE)
		tile_size = MAX_TILE_SIZE;

	return tile_size;
}

void matrix_set(struct matrix *ptr, vamp_t lmin, vamp_t lmax)
{
	assert(lmin <= lmax);
	assert(ptr->arr == NULL);

	ptr->unfinished = 0;

	length_t fang_length = length(lmin) / 2;
	if (fang_length == length(fang_max))
		ptr->fmax = fang_max;
	else
		ptr->fmax = pow10v(fang_length); // Max factor value.

	matrix_init_cleanup(ptr);

	if (ptr->fmax < fang_max) {
		vamp_t fmaxsquare = ptr->fmax;
		fmaxsquare *= ptr->fmax;
		if (lmax > fmaxsquare && lmin <= fmaxsquare)
			lmax = fmaxsquare; // Max can be bigger than fmax ^ 2: 9999 > 99 ^ 2.
	}

	vamp_t tile_size = get_tilesize(lmin, lmax);

	ptr->size = div_roof((lmax - lmin + 1), tile_size + (tile_size < vamp_max));
	ptr->arr = malloc(sizeof(struct tile *) * ptr->size);
	if (ptr->arr == NULL)
		abort();

	vamp_t x = 0;
	vamp_t iterator = tile_size;
	for (vamp_t i = lmin; i <= lmax; i += iterator + 1) {
		if (lmax - i < tile_size)
			iterator = lmax - i;

		ptr->arr[x++] = tile_init(i, i + iterator);

		if (i == lmax)
			break;
		if (i + iterator == vamp_max)
			break;
	}
	ptr->arr[ptr->size - 1]->lmax = lmax;
}

void matrix_reset(struct matrix *ptr)
{
	for (vamp_t i = 0; i < ptr->size; i++)
		tile_free(ptr->arr[i]);
	free(ptr->arr);
	ptr->arr = NULL;
}

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)

void matrix_print(struct matrix *ptr, vamp_t *count)
{
	for (vamp_t x = ptr->cleanup; x < ptr->size; x++)
		if (ptr->arr[x] != NULL) {
			llhandle_print(ptr->arr[x]->result, *count);
			*count += ptr->arr[x]->result->size;
		}
}

#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */

#if defined(PROCESS_RESULTS) && DISPLAY_PROGRESS

// matrix_progress requires mutex lock
void matrix_progress( struct matrix *ptr)
{
	fprintf(stderr, "%llu, %llu", ptr->arr[ptr->cleanup]->lmin, ptr->arr[ptr->cleanup]->lmax);
	fprintf(stderr, "  %llu/%llu\n", ptr->cleanup + 1, ptr->size);
}

#endif /* defined(PROCESS_RESULTS) && DISPLAY_PROGRESS */