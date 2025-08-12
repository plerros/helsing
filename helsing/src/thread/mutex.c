// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#if (THREADS_PTHREAD)

#include <stdlib.h>
#include "helper.h"
#include "mutex.h"

void mutex_new(struct mutex_t **ptr)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct mutex_t *new = malloc(sizeof(struct mutex_t));
	if (new == NULL)
		abort();

	new->mtx = malloc(sizeof(pthread_mutex_t));
	if (new->mtx == NULL)
		abort();

	pthread_mutex_init(new->mtx, NULL);
	*ptr = new;
}

void mutex_free(struct mutex_t *ptr)
{
	pthread_mutex_destroy(ptr->mtx);
	free(ptr->mtx);
	free(ptr);
}

void mutex_lock(struct mutex_t *ptr)
{
	pthread_mutex_lock(ptr->mtx);
}

void mutex_unlock(struct mutex_t *ptr)
{
	pthread_mutex_unlock(ptr->mtx);
}

#endif