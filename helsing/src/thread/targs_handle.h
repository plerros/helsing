// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TARGS_HANDLE_H
#define HELSING_TARGS_HANDLE_H

#include "configuration.h"
#include "taskboard.h"
#include "cache.h"
#include "targs.h"
#include "mutex.h"

struct targs_handle
{
	struct options_t options;
	struct targs **targs;
	struct taskboard *progress;
	struct cache *digptr;
	struct mutex_t *read;
	struct mutex_t *write;
};

void targs_handle_new(struct targs_handle **ptr, struct options_t options, vamp_t min, vamp_t max, struct taskboard *progress);
void targs_handle_free(struct targs_handle *ptr);
void targs_handle_print(struct targs_handle *ptr);
#endif /* HELSING_TARGS_HANDLE_H */
