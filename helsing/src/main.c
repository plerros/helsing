// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>	// isdigit

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"
#include "taskboard.h"
#include "targs_handle.h"
#include "checkpoint.h"
#include "interval.h"

static int strtov(const char *str, vamp_t *number) // string to vamp_t
{
	assert(str != NULL);
	assert(number != NULL);
	vamp_t ret = 0;
	for (length_t i = 0; isgraph(str[i]); i++) {
		if (!isdigit(str[i]))
			return 1;
		digit_t digit = str[i] - '0';
		if (willoverflow(ret, digit))
			return 1;
		ret = 10 * ret + digit;
	}
	*number = ret;
	return 0;
}

static vamp_t get_lmax(vamp_t lmin, vamp_t max)
{
	if (length(lmin) < length(vamp_max)) {
		vamp_t lmax = pow_v(length(lmin)) - 1;
		if (lmax < max)
			return lmax;
	}
	return max;
}

static bool check_argc (int argc)
{
#if !USE_CHECKPOINT
	return (argc != 3);
#else
	return (argc != 1 && argc != 3);
#endif
}

int main(int argc, char *argv[])
{
	struct interval_t interval;
	struct taskboard *progress = NULL;

	vamp_t min, max;
	if (check_argc(argc)) {
		printf("Usage: helsing [min] [max]\n");
		goto out;
	}
	if (argc == 3) {
		if (strtov(argv[1], &min) || strtov(argv[2], &max)) {
			fprintf(stderr, "Input out of interval: [0, %llu]\n", vamp_max);
			goto out;
		}
		if (interval_set(&interval, min, max))
			goto out;
		if (touch_checkpoint(interval))
			goto out;
	}

	taskboard_new(&progress, &interval);

	if (USE_CHECKPOINT) {
		if (load_checkpoint(&interval, progress))
			goto out;
	}

	pthread_t threads[THREADS];
	struct targs_handle *thhandle = NULL;
	targs_handle_new(&(thhandle), interval.max, progress);

	for (; interval.complete < interval.max;) {
		vamp_t lmin = get_min(interval.complete + 1,  interval.max);
		vamp_t lmax = get_lmax(lmin, interval.max);
		taskboard_set(progress, lmin, lmax);
		fprintf(stderr, "Checking interval: [%llu, %llu]\n", lmin, lmax);
		for (thread_t thread = 0; thread < THREADS; thread++)
			assert(pthread_create(&threads[thread], NULL, thread_function, (void *)(thhandle->targs[thread])) == 0);
		for (thread_t thread = 0; thread < THREADS; thread++)
			pthread_join(threads[thread], 0);

	}
	targs_handle_print(thhandle);
	targs_handle_free(thhandle);
out:
	taskboard_free(progress);
	return 0;
}
