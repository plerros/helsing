// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdbool.h>

#include "configuration.h"
#include "task.h"
#include "llhandle.h"

struct task *task_init(vamp_t lmin, vamp_t lmax)
{
	struct task *new = malloc(sizeof(struct task));
	if (new == NULL)
		abort();

	new->lmin = lmin;
	new->lmax = lmax;
	new->result = NULL;
	return new;
}

void task_free(struct task *ptr)
{
	if (ptr != NULL)
		llhandle_free(ptr->result);
	free(ptr);
}