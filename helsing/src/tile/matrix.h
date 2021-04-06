// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_MATRIX_H
#define HELSING_MATRIX_H

#include "configuration.h"
#include "tile.h"

struct matrix
{
	struct tile **arr;
	vamp_t size;
	vamp_t unfinished; // First tile that hasn't been accepted.
	vamp_t cleanup;    // Last tile that hasn't been processed.
	fang_t fmax;
};

#ifdef PROCESS_RESULTS
static inline void matrix_init_cleanup(struct matrix *ptr)
{
	ptr->cleanup = 0;
}
#else /* PROCESS_RESULTS */
static inline void matrix_init_cleanup(__attribute__((unused)) struct matrix *ptr)
{
}
#endif /* PROCESS_RESULTS */

struct matrix *matrix_init();
void matrix_free(struct matrix *ptr);

void matrix_set(struct matrix *ptr, vamp_t lmin, vamp_t lmax);
void matrix_reset(struct matrix *ptr);

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void matrix_print(struct matrix *ptr, vamp_t *count);
#else /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */
static inline void matrix_print(
	__attribute__((unused)) struct matrix *ptr,
	__attribute__((unused)) vamp_t *count)
{
}
#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */

#if defined(PROCESS_RESULTS) && DISPLAY_PROGRESS
void matrix_progress( struct matrix *ptr);
#else /* defined(PROCESS_RESULTS) && DISPLAY_PROGRESS */
static inline void matrix_progress(__attribute__((unused)) struct matrix *ptr)
{
}
#endif /* defined(PROCESS_RESULTS) && DISPLAY_PROGRESS */
#endif /* HELSING_MATRIX_H */