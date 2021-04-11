// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "configuration.h"
#include "llhandle.h"
#include "taskboard.h"
#include "cache.h"
#include "vargs.h"
#include "targs.h"
#include "targs_handle.h"

struct targs_handle *targs_handle_init(vamp_t max)
{
	struct targs_handle *new = malloc(sizeof(struct targs_handle));
	if (new == NULL)
		abort();

	new->progress = taskboard_init();
	cache_init(&(new->digptr), max);

	new->read = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->read, NULL);
	new->write = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->write, NULL);

	for (thread_t thread = 0; thread < THREADS; thread++)
		new->targs[thread] = targs_t_init(new->read, new->write, new->progress, new->digptr);
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
	fprintf(stderr, "Found: %llu valid fang pairs.\n", ptr->progress->common_count);
#else
	fprintf(stderr, "Found: %llu vampire numbers.\n",  ptr->progress->common_count);
#endif

#ifdef CHECKSUM_RESULTS
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len;

	EVP_DigestFinal_ex(ptr->progress->common_mdctx, md_value, &md_len);
	EVP_MD_CTX_destroy(ptr->progress->common_mdctx);
	ptr->progress->common_mdctx = NULL;

	printf("Digest %s is: ", DIGEST_NAME);
	for(unsigned int i = 0; i < md_len; i++)
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

	do {
		current = NULL;
// Critical section start
		pthread_mutex_lock(args->read);

		current = taskboard_get_task(args->progress);

		pthread_mutex_unlock(args->read);
// Critical section end

		if (current != NULL) {
			vampire(current->lmin, current->lmax, vamp_args, args->progress->fmax);

// Critical section start
			pthread_mutex_lock(args->write);

			task_copy_vargs(current, vamp_args);
#if MEASURE_RUNTIME
			args->total += current->count;
#endif
			taskboard_cleanup(args->progress);

			pthread_mutex_unlock(args->write);
// Critical section end
			vargs_reset(vamp_args);
		}
	} while (current != NULL);
	vargs_free(vamp_args);
	thread_timer_stop(args);
	return 0;
}
