// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025 Pierro Zachareas
 */

#ifndef HELSING_THREAD_H
#define HELSING_THREAD_H

#include "configuration.h"
#include "options.h"
#include "targs_handle.h"

#if (THREADS_PTHREAD)

struct threads_t
{
	pthread_t *threads;
};

#else /* PTHREADS */

struct threads_t{};

#endif /* PTHREADS */

void threads_new(struct threads_t **ptr, struct options_t *options);
void threads_free(struct threads_t *ptr);
void threads_create(struct threads_t *ptr, struct options_t *options, struct targs_handle *thhandle);
void threads_join(struct threads_t *ptr, struct options_t *options);

#endif /* HELSING_THREAD_H */