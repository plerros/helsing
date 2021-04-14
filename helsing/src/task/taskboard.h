// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TASKBOARD_H
#define HELSING_TASKBOARD_H

#include "configuration.h"
#include "task.h"
#include "hash.h"

struct taskboard
{
	struct task **tasks;
	vamp_t size; // The size of the tasks array
	vamp_t todo; // First task that hasn't been accepted.
	vamp_t done; // Last task that's completed, but isn't yet processed. (print, hash, checksum...)
	fang_t fmax;
	vamp_t common_count;
	struct hash *checksum;
};

struct taskboard *taskboard_init();
void taskboard_free(struct taskboard *ptr);
void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax);
void taskboard_reset(struct taskboard *ptr);
struct task *taskboard_get_task(struct taskboard *ptr);
void taskboard_cleanup(struct taskboard *ptr);
void taskboard_print_results(struct taskboard *ptr);

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void taskboard_print(struct taskboard *ptr);
#else /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */
static inline void taskboard_print(__attribute__((unused)) struct taskboard *ptr)
{
}
#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */

#if DISPLAY_PROGRESS
void taskboard_progress(struct taskboard *ptr);
#else /* DISPLAY_PROGRESS */
static inline void taskboard_progress(__attribute__((unused)) struct taskboard *ptr)
{
}
#endif /* DISPLAY_PROGRESS */
#endif /* HELSING_TASKBOARD_H */