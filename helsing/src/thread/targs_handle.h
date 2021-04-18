// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TARGS_HANDLE_H
#define HELSING_TARGS_HANDLE_H

#include <pthread.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "taskboard.h"
#include "cache.h"
#include "targs.h"

struct targs_handle
{
	struct targs *targs[THREADS];
	struct taskboard *progress;
	struct cache *digptr;
	pthread_mutex_t *read;
	pthread_mutex_t *write;
};

void targs_handle_new(struct targs_handle **ptr, vamp_t max, struct taskboard *progress);
void targs_handle_free(struct targs_handle *ptr);
void targs_handle_print(struct targs_handle *ptr);
#endif /* HELSING_TARGS_HANDLE_H */