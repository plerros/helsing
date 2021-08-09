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

static vamp_t get_min(vamp_t min, vamp_t max)
{
	if (length(min) % 2) {
		length_t min_length = length(min);
		if (min_length < length(max))
			min = pow10v(min_length);
		else
			min = max;
	}
	return min;
}

static vamp_t get_max(vamp_t min, vamp_t max)
{
	if (length(min) % 2) {
		length_t max_length = length(max);
		if (max_length > length(min))
			max = pow10v(max_length - 1) - 1;
		else
			max = min;
	}
	return max;
}

static vamp_t get_lmax(vamp_t lmin, vamp_t max)
{
	if (length(lmin) < length(vamp_max)) {
		vamp_t lmax = pow10v(length(lmin)) - 1;
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

	if (check_argc(argc)) {
		printf("Usage: helsing [min] [max]\n");
		return 0;
	}
	if (argc == 3) {
		if (strtov(argv[1], &min) || strtov(argv[2], &max)) {
			fprintf(stderr, "Input out of range: [0, %llu]\n", vamp_max);
			return 0;
		}
		if (touch_checkpoint(min, max))
			return 0;
	}
	struct taskboard *progress = NULL;
	taskboard_new(&(progress));

	if (argc == 1) {
		vamp_t ccurrent;
		if (load_checkpoint(&min, &max, &ccurrent, progress)) {
			taskboard_free(progress);
			return 0;
		}
		if (ccurrent == max) {
			taskboard_print_results(progress);
			taskboard_free(progress);
			return 0;
		}
		if (ccurrent > min)
			min = ccurrent + 1;
	}

	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		taskboard_free(progress);
		return 0;
	}
	if (max > 9999999999 && ELEMENT_BITS == 32) {
		fprintf(stderr, "WARNING: the code might produce false ");
		fprintf(stderr, "positives, please set ELEMENT_BITS to 64.\n");
		taskboard_free(progress);
		return 0;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	vamp_t lmin = min;
	vamp_t lmax = get_lmax(lmin, max);

	pthread_t threads[THREADS];
	struct targs_handle *thhandle = NULL;
	targs_handle_new(&(thhandle), max, progress);

	for (; lmax <= max;) {
		fprintf(stderr, "Checking range: [%llu, %llu]\n", lmin, lmax);
		taskboard_set(thhandle->progress, lmin, lmax);
		for (thread_t thread = 0; thread < THREADS; thread++)
			assert(pthread_create(&threads[thread], NULL, thread_function, (void *)(thhandle->targs[thread])) == 0);
		for (thread_t thread = 0; thread < THREADS; thread++)
			pthread_join(threads[thread], 0);

		taskboard_reset(thhandle->progress);
		if (lmax == max)
			break;

		lmin = get_min(lmax + 1, max);
		lmax = get_lmax(lmin, max);
	}
	targs_handle_print(thhandle);
	targs_handle_free(thhandle);
	return 0;
}