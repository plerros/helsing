// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025 Pierro Zachareas
 */

#ifndef HELSING_MUTEX_H
#define HELSING_MUTEX_H

#include "configuration.h"

#if (THREADS_PTHREAD)

#include <pthread.h>

struct mutex_t
{
	pthread_mutex_t *mtx;
};

void mutex_new(struct mutex_t **ptr);
void mutex_free(struct mutex_t *ptr);
void mutex_lock(struct mutex_t *ptr);
void mutex_unlock(struct mutex_t *ptr);

#else /* PTHREADS */

struct mutex_t {};
static inline void mutex_new   (__attribute__((unused)) struct mutex_t **ptr){}
static inline void mutex_free  (__attribute__((unused)) struct mutex_t *ptr){}
static inline void mutex_lock  (__attribute__((unused)) struct mutex_t *ptr){}
static inline void mutex_unlock(__attribute__((unused)) struct mutex_t *ptr){}

#endif /* PTHREADS */

#endif /* HELSING_MUTEX_H */