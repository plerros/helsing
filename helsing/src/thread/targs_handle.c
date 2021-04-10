// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <openssl/evp.h>

#include "configuration.h"
#include "llhandle.h"
#include "task.h"
#include "taskboard.h"
#include "cache.h"
#include "vargs.h"
#include "targs.h"
#include "targs_handle.h"
#include "checkpoint.h"

struct targs_handle *targs_handle_init(vamp_t max)
{
	struct targs_handle *new = malloc(sizeof(struct targs_handle));
	if (new == NULL)
		abort();

	new->progress = taskboard_init();
	cache_init(&(new->digptr), max);
	new->counter = 0;
	new->read = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->read, NULL);
	new->write = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->write, NULL);

	const EVP_MD *md;
	OpenSSL_add_all_digests();

	#ifdef CHECKSUM_RESULTS
	md = EVP_get_digestbyname(DIGEST_NAME);
	#else
	md = EVP_md_null();
	#endif

	if(!md) {
		printf("Unknown message digest %s\n", DIGEST_NAME);
		exit(1);
	}
	new->mdctx = EVP_MD_CTX_create();

	#ifdef CHECKSUM_RESULTS
	EVP_DigestInit_ex(new->mdctx, md, NULL);
	#endif

	for (thread_t thread = 0; thread < THREADS; thread++)
		new->targs[thread] = targs_t_init(new->read, new->write, new->progress, &(new->counter), new->digptr, new->mdctx);
	return new;
}

void targs_handle_free(struct targs_handle *ptr)
{
	if (ptr == NULL)
		return;

	pthread_mutex_destroy(ptr->read);
	free(ptr->read);
	pthread_mutex_destroy(ptr->write);
	free(ptr->write);
	taskboard_free(ptr->progress);
	cache_free(ptr->digptr);
	free(ptr->mdctx);

	EVP_cleanup();

	for (thread_t thread = 0; thread < THREADS; thread++)
		targs_t_free(ptr->targs[thread]);

	free(ptr);
}

void targs_handle_print(struct targs_handle *ptr)
{
#if MEASURE_RUNTIME
	double total_time = 0.0;
	fprintf(stderr, "Thread  Runtime Count\n");
	for (thread_t thread = 0; thread<THREADS; thread++) {
		fprintf(stderr, "%u\t%.2lfs\t%llu\n", thread, ptr->targs[thread]->runtime, ptr->targs[thread]->total);
		total_time += ptr->targs[thread]->runtime;
	}
	fprintf(stderr, "\nFang search took: %.2lf s, average: %.2lf s\n", total_time, total_time / THREADS);
#endif

#if defined COUNT_RESULTS ||  defined DUMP_RESULTS
	fprintf(stderr, "Found: %llu valid fang pairs.\n", ptr->counter);
#else
	fprintf(stderr, "Found: %llu vampire numbers.\n", ptr->counter);
#endif

#ifdef CHECKSUM_RESULTS
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len, i;

	EVP_DigestFinal_ex(ptr->mdctx, md_value, &md_len);
	EVP_MD_CTX_destroy(ptr->mdctx);
	ptr->mdctx = NULL;

	printf("Digest %s is: ", DIGEST_NAME);
	for(i = 0; i < md_len; i++)
		printf("%02x", md_value[i]);
	printf("\n");
#endif
}

void *thread_worker(void *void_args)
{
	struct targs_t *args = (struct targs_t *)void_args;
	thread_timer_start(args);
	struct vargs *vamp_args = vargs_init(args->digptr);
	struct task *current = NULL;
	bool active = true;

	while (active) {
		active = false;

// Critical section start
		pthread_mutex_lock(args->read);
		if (args->progress->todo < args->progress->size) {
			current = args->progress->tasks[args->progress->todo];
			active = true;
			args->progress->todo += 1;
		}
		pthread_mutex_unlock(args->read);
// Critical section end

		if (active) {
			vampire(current->lmin, current->lmax, vamp_args, args->progress->fmax);

// Critical section start
			pthread_mutex_lock(args->write);
#ifdef PROCESS_RESULTS
			current->result = vargs_getlhandle(vamp_args);
			while (
				args->progress->done < args->progress->size &&
				args->progress->tasks[args->progress->done]->result != NULL)
			{
				llhandle_print(args->progress->tasks[args->progress->done]->result, *(args->count));
				llhandle_checksum(args->progress->tasks[args->progress->done]->result, args->mdctx);

				*(args->count) += args->progress->tasks[args->progress->done]->result->size;
				taskboard_progress(args->progress);

				save_checkpoint(args->progress->tasks[args->progress->done]->lmax, *(args->count));

				task_free(args->progress->tasks[args->progress->done]);
				args->progress->tasks[args->progress->done] = NULL;
				args->progress->done += 1;
			}
#else
			*(args->count) += vamp_args->local_count;
#endif
			pthread_mutex_unlock(args->write);
// Critical section end
			vargs_reset(vamp_args);
		}
	}
#if MEASURE_RUNTIME
	args->total += vamp_args->total;
#endif
	vargs_free(vamp_args);
	thread_timer_stop(args);
	return 0;
}
