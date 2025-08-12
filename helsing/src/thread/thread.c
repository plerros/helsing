// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#include <assert.h>
#include <stdlib.h>
#include "thread.h"

#if (THREADS_PTHREAD)
#include <pthread.h>

void threads_new(struct threads_t **ptr, struct options_t *options)
{
	struct threads_t *new = malloc(sizeof(struct threads_t));
	if (new == NULL)
		abort();

	new->threads = malloc(sizeof(pthread_t) * options->threads);
	if (new->threads == NULL)
		abort();
	
	*ptr = new;
}

void threads_free(struct threads_t *ptr)
{
	free(ptr->threads);
	free(ptr);
}

void threads_create(struct threads_t *ptr, struct options_t *options, struct targs_handle *thhandle)
{
	for (thread_t thread = 0; thread < options->threads; thread++)
		assert(pthread_create(&(ptr->threads)[thread], NULL, thread_function, (void *)(thhandle->targs[thread])) == 0);
}

void threads_join(struct threads_t *ptr, struct options_t *options)
{
	for (thread_t thread = 0; thread < options->threads; thread++)
		pthread_join(ptr->threads[thread], 0);
}

#else /* THREADS_PTHREAD */

void threads_new(
	__attribute__((unused)) struct threads_t **ptr,
	__attribute__((unused)) struct options_t *options)
{
}

void threads_free(
	__attribute__((unused)) struct threads_t *ptr)
{
}

void threads_create(
	__attribute__((unused)) struct threads_t *ptr,
	struct options_t *options,
	struct targs_handle *thhandle)
{
	for (thread_t thread = 0; thread < options->threads; thread++)
		thread_function((void *)(thhandle->targs[thread]));
}

void threads_join(
	__attribute__((unused)) struct threads_t *ptr,
	__attribute__((unused)) struct options_t *options)
{
}

#endif /* THREADS_PTHREAD */