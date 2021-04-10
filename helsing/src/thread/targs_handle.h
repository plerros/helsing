// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TARGS_HANDLE_H
#define HELSING_TARGS_HANDLE_H

#include <pthread.h>
#include <openssl/evp.h>

#include "configuration.h"
#include "taskboard.h"
#include "cache.h"
#include "targs.h"

struct targs_handle
{
	struct targs_t *targs[THREADS];
	struct taskboard *progress;
	struct cache *digptr;
	EVP_MD_CTX *mdctx;
	vamp_t counter;
	pthread_mutex_t *read;
	pthread_mutex_t *write;
};

struct targs_handle *targs_handle_init(vamp_t max);
void targs_handle_free(struct targs_handle *ptr);
void targs_handle_print(struct targs_handle *ptr);

void *thread_worker(void *void_args);
#endif /* HELSING_TARGS_HANDLE_H */