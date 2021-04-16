// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>

#include "configuration.h"
#include "task.h"
#include "llhandle.h"
#include "vargs.h"

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
#include <assert.h>
#endif

void task_new(struct task **ptr, vamp_t lmin, vamp_t lmax)
{
	if (ptr == NULL)
		return;

	struct task *new = malloc(sizeof(struct task));
	if (new == NULL)
		abort();

	new->lmin = lmin;
	new->lmax = lmax;
	new->result = NULL;
	new->count = 0;
	*ptr = new;
}

void task_free(struct task *ptr)
{
	if (ptr == NULL)
		return;

	llhandle_free(ptr->result);
	free(ptr);
}

void task_copy_vargs(struct task *ptr, struct vargs *vamp_args)
{
	ptr->result = vamp_args->lhandle;
	ptr->count = vamp_args->local_count;

	vamp_args->lhandle = NULL;
}

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)

void task_print(struct task *ptr, vamp_t *count)
{
#if SANITY_CHECK
	assert(ptr != NULL);
#endif
	llhandle_print(ptr->result, *count);
	*count += ptr->result->size;
}

#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */