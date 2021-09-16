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
	vamp_t min, max;
	vamp_t complete = 0;
	struct taskboard *progress = NULL;

	if (check_argc(argc)) {
		printf("Usage: helsing [min] [max]\n");
		goto out;
	}
	if (argc == 3) {
		if (strtov(argv[1], &min) || strtov(argv[2], &max)) {
			fprintf(stderr, "Input out of interval: [0, %llu]\n", vamp_max);
			goto out;
		}
		if (touch_checkpoint(min, max))
			goto out;
	}
	taskboard_new(&(progress));

	if (argc == 1) {
		if (load_checkpoint(&min, &max, &complete, progress))
			goto out;
	}

	if (complete < min)
		complete = min - 1;
	
	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		goto out;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	if (cache_ovf_chk(max)) {
		fprintf(stderr, "WARNING: the code might produce false positives, ");
		if (ELEMENT_BITS == 32)
			fprintf(stderr, "please set ELEMENT_BITS to 64.\n");
		else
			fprintf(stderr, "please set CACHE to false.\n");
		goto out;
	}

	pthread_t threads[THREADS];
	struct targs_handle *thhandle = NULL;
	targs_handle_new(&(thhandle), max, progress);

	for (; complete < max;) {
		vamp_t lmin = get_min(complete + 1, max);
		vamp_t lmax = get_lmax(lmin, max);
		taskboard_set(progress, lmin, lmax);
		fprintf(stderr, "Checking interval: [%llu, %llu]\n", lmin, lmax);
		for (thread_t thread = 0; thread < THREADS; thread++)
			assert(pthread_create(&threads[thread], NULL, thread_function, (void *)(thhandle->targs[thread])) == 0);
		for (thread_t thread = 0; thread < THREADS; thread++)
			pthread_join(threads[thread], 0);

		taskboard_reset(progress);
		complete = lmax;
	}
	targs_handle_print(thhandle);
	targs_handle_free(thhandle);
out:
	taskboard_free(progress);
	return 0;
}
