// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2026 Pierro Zachareas
 */

#ifndef HELSING_TARGS_HANDLE_H
#define HELSING_TARGS_HANDLE_H

#include <threads.h>

#include "datatypes.h"

#include "taskboard.h"
#include "cache.h"
#include "targs.h"

struct targs_handle
{
	struct options_t options;
	struct targs **targs;
	struct taskboard *progress;
	struct cache *digptr;
	mtx_t *read;
	mtx_t *write;
	mtx_t *stdout_mtx;
};

void targs_handle_new(struct targs_handle **ptr, struct options_t options, vamp_t min, vamp_t max, struct taskboard *progress);
void targs_handle_free(struct targs_handle *ptr);
void targs_handle_print(struct targs_handle *ptr);
#endif /* HELSING_TARGS_HANDLE_H */
