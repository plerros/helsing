// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2026 Pierro Zachareas
 */

#ifndef HELSING_TASKBOARD_H
#define HELSING_TASKBOARD_H

#include <threads.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "task.h"
#include "options.h"
#include "hash.h"

struct taskboard
{
	struct options_t options;
	struct task **tasks;
	size_t size; // The size of the tasks array
	size_t todo; // First task that hasn't been accepted.
	size_t done; // Last task that's completed, but isn't yet processed. (print, hash, checksum...)
	vamp_t lmin; // Copy of the lmin value
	vamp_t lmax; // Copy of the lmax value
	fang_t fmax;
	vamp_t interval_size;
	vamp_t common_count[COUNT_ARRAY_SIZE];
	vamp_t common_prev[COUNT_ARRAY_SIZE]; // The last vampire number that got printed out.
	struct hash *checksum;
};

void taskboard_new(struct taskboard **ptr, struct options_t options);
void taskboard_free(struct taskboard *ptr);
void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax);
struct task *taskboard_get_task(struct taskboard *ptr);
void taskboard_cleanup(struct taskboard *ptr, mtx_t *stdout_mtx);
void taskboard_print_results(struct taskboard *ptr);
void taskboard_progress(struct taskboard *ptr, mtx_t *stdout_mtx);
#endif /* HELSING_TASKBOARD_H */
