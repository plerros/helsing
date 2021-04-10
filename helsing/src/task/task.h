// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TASK_H
#define HELSING_TASK_H

#include "configuration.h"

#ifdef PROCESS_RESULTS
#include "llhandle.h"
#endif

/*
 * task:
 *
 * A task consists of a closed interval [a, b] and a pointer to a linked list,
 * where the results will be stored.
 */

struct task
{
	vamp_t lmin; // local minimum
	vamp_t lmax; // local maximum
	struct llhandle *result;
};

struct task *task_init(vamp_t lmin, vamp_t lmax);
void task_free(struct task *ptr);
#endif /* HELSING_TASK_H */