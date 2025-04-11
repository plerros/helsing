// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_OPTIONS_H
#define HELSING_OPTIONS_H

#include <stdbool.h>

#include "configuration_adv.h"

struct options_t
{
	vamp_t min;
	vamp_t max;
	thread_t threads;
	vamp_t manual_task_size;
	bool display_progress;
	bool load_checkpoint;
	char *checkpoint;
	bool dry_run;
};

int options_new(struct options_t **ptr, int argc, char *argv[]);
void options_free(struct options_t *ptr);
bool options_touch_checkpoint(struct options_t options);

#endif /* HELSING_OPTIONS_H */
