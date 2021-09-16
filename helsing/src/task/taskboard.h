// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TASKBOARD_H
#define HELSING_TASKBOARD_H

#include "configuration.h"
#include "configuration_adv.h"
#include "task.h"
#include "hash.h"
#include "interval.h"

struct taskboard
{
	struct interval_t *interval;
	struct task **tasks;
	vamp_t size; // The size of the tasks array
	vamp_t todo; // First task that hasn't been accepted.
	vamp_t done; // Last task that's completed, but isn't yet processed. (print, hash, checksum...)
	fang_t fmax;
	vamp_t common_count;
	struct hash *checksum;
};

void taskboard_new(struct taskboard **ptr, struct interval_t *interval);
void taskboard_free(struct taskboard *ptr);
void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax);
struct task *taskboard_get_task(struct taskboard *ptr);
void taskboard_cleanup(struct taskboard *ptr);
void taskboard_print_results(struct taskboard *ptr);

#if DISPLAY_PROGRESS
void taskboard_progress(struct taskboard *ptr);
#else /* DISPLAY_PROGRESS */
static inline void taskboard_progress(__attribute__((unused)) struct taskboard *ptr)
{
}
#endif /* DISPLAY_PROGRESS */
#endif /* HELSING_TASKBOARD_H */
