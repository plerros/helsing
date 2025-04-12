// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_TASK_H
#define HELSING_TASK_H

#include <stdbool.h>
#include "configuration_adv.h"
#include "vargs.h"
#include "array.h"

/*
 * task:
 *
 * A task consists of a closed interval [lmin, lmax] and a pointer to an array,
 * where the results will be stored.
 */

struct task
{
	vamp_t lmin; // local minimum
	vamp_t lmax; // local maximum
	struct array *result;
	vamp_t count[COUNT_ARRAY_SIZE];
	bool complete;
};

void task_new(struct task **ptr, vamp_t lmin, vamp_t lmax);
void task_free(struct task *ptr);
void task_copy_vargs(struct task *ptr, struct vargs *vamp_args);
#endif /* HELSING_TASK_H */
