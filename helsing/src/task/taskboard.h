// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TASKBOARD_H
#define HELSING_TASKBOARD_H

#include "configuration.h"
#include "task.h"

struct taskboard
{
	struct task **tasks;
	vamp_t size; // The size of the tasks array
	vamp_t todo; // First task that hasn't been accepted.
	vamp_t done; // Last task that's completed, but isn't yet processed. (print, hash, checksum...)
	fang_t fmax;
};

#ifdef PROCESS_RESULTS
static inline void taskboard_init_done(struct taskboard *ptr)
{
	ptr->done = 0;
}
#else /* PROCESS_RESULTS */
static inline void taskboard_init_done(__attribute__((unused)) struct taskboard *ptr)
{
}
#endif /* PROCESS_RESULTS */

struct taskboard *taskboard_init();
void taskboard_free(struct taskboard *ptr);
void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax);
void taskboard_reset(struct taskboard *ptr);

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void taskboard_print(struct taskboard *ptr, vamp_t *count);
#else /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */
static inline void taskboard_print(
	__attribute__((unused)) struct taskboard *ptr,
	__attribute__((unused)) vamp_t *count)
{
}
#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */

#if defined(PROCESS_RESULTS) && DISPLAY_PROGRESS
void taskboard_progress( struct taskboard *ptr);
#else /* defined(PROCESS_RESULTS) && DISPLAY_PROGRESS */
static inline void taskboard_progress(__attribute__((unused)) struct taskboard *ptr)
{
}
#endif /* defined(PROCESS_RESULTS) && DISPLAY_PROGRESS */
#endif /* HELSING_TASKBOARD_H */