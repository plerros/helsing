// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_TASKBOARD_H
#define HELSING_TASKBOARD_H

#include "configuration.h"
#include "configuration_adv.h"
#include "task.h"
#include "options.h"
#include "hash.h"

struct taskboard
{
	struct options_t options;
	struct task **tasks;
	vamp_t size; // The size of the tasks array
	vamp_t todo; // First task that hasn't been accepted.
	vamp_t done; // Last task that's completed, but isn't yet processed. (print, hash, checksum...)
	fang_t fmax;
	vamp_t common_count[COUNT_ARRAY_SIZE];
	vamp_t common_prev[COUNT_ARRAY_SIZE]; // The last vampire number that got printed out.
	struct hash *checksum;
};

void taskboard_new(struct taskboard **ptr, struct options_t options);
void taskboard_free(struct taskboard *ptr);
void taskboard_set(struct taskboard *ptr, vamp_t lmin, vamp_t lmax);
struct task *taskboard_get_task(struct taskboard *ptr);
void taskboard_cleanup(struct taskboard *ptr);
void taskboard_print_results(struct taskboard *ptr);
void taskboard_progress(struct taskboard *ptr);
#endif /* HELSING_TASKBOARD_H */
