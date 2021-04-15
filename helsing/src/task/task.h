// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TASK_H
#define HELSING_TASK_H

#include "configuration.h"
#include "vargs.h"

#include "llhandle.h"

/*
 * task:
 *
 * A task consists of a closed interval [lmin, lmax] and a pointer to a linked
 * list, where the results will be stored.
 */

struct task
{
	vamp_t lmin; // local minimum
	vamp_t lmax; // local maximum
	struct llhandle *result;
	vamp_t count;
};

void task_new(struct task **ptr, vamp_t lmin, vamp_t lmax);
void task_free(struct task *ptr);
void task_copy_vargs(struct task *ptr, struct vargs *vamp_args);

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void task_print(struct task *ptr, vamp_t *count);
#else /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */
static inline void task_print(
	__attribute__((unused)) struct task *ptr,
	__attribute__((unused)) vamp_t *count)
{
}
#endif /* defined(PROCESS_RESULTS) && defined(PRINT_RESULTS) */
#endif /* HELSING_TASK_H */