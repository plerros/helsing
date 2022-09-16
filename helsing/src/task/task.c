// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdbool.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"
#include "task.h"
#include "array.h"
#include "vargs.h"

void task_new(struct task **ptr, vamp_t lmin, vamp_t lmax)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct task *new = malloc(sizeof(struct task));
	if (new == NULL)
		abort();

	new->lmin = lmin;
	new->lmax = lmax;
	new->result = NULL;
	new->count = 0;
	new->complete = false;
	*ptr = new;
}

void task_free(struct task *ptr)
{
	if (ptr == NULL)
		return;

	array_free(ptr->result);
	free(ptr);
}

void task_copy_vargs(struct task *ptr, struct vargs *vamp_args)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(vamp_args != NULL);

	ptr->result = vamp_args->result;
	ptr->count = vamp_args->local_count;
	ptr->complete = true;

	vamp_args->result = NULL;
}
