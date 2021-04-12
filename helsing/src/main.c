// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>	// isdigit

#include "configuration.h"
#include "helper.h"
#include "taskboard.h"
#include "targs_handle.h"
#include "checkpoint.h"

static bool length_isodd(vamp_t x)
{
	return (length(x) % 2);
}

static int atov(const char *str, vamp_t *number) // ASCII to vamp_t
{
	assert(str != NULL);
	assert(number != NULL);
	vamp_t ret = 0;
	for (length_t i = 0; isdigit(str[i]); i++) {
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
	if (length_isodd(min)) {
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
	if (length_isodd(max)) {
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
	vamp_t lmax;
	if (length(lmin) < length(vamp_max)) {
		lmax = pow10v(length(lmin)) - 1;
		if (lmax < max)
			return lmax;
	}
	return max;
}

int main(int argc, char* argv[])
{
	vamp_t min, max;
#if !defined(USE_CHECKPOINT) || !USE_CHECKPOINT
	if (argc != 3) {
		printf("Usage: helsing [min] [max]\n");
		return 0;
	}
	if (atov(argv[1], &min) || atov(argv[2], &max)) {
		fprintf(stderr, "Input out of range: [0, %llu]\n", vamp_max);
		return 1;
	}
#else
	vamp_t count = 0;
	unsigned char mdtmp[EVP_MAX_MD_SIZE] = {0};
	if (argc != 1 && argc != 3) {
		printf("Usage: helsing [min] [max]\n");
		printf("to recover from %s: helsing\n", CHECKPOINT_FILE);
		return 0;
	}
	if (argc == 1) {
		vamp_t ccurrent;
		load_checkpoint(&min, &max, &ccurrent, &count, mdtmp);
		if (ccurrent > min)
			min = ccurrent + 1;
	}
	if (argc == 3) {
		if (atov(argv[1], &min) || atov(argv[2], &max)) {
			fprintf(stderr, "Input out of range: [0, %llu]\n", vamp_max);
			return 1;
		}
		touch_checkpoint(min, max);
	}
#endif
	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		return 1;
	}
	if (max > 9999999999 && ELEMENT_BITS == 32) {
		fprintf(stderr, "WARNING: the code might produce false ");
		fprintf(stderr, "positives, please set ELEMENT_BITS to 64.\n");
		return 0;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	vamp_t lmin = min;
	vamp_t lmax = get_lmax(lmin, max);

	pthread_t threads[THREADS];
	struct targs_handle *thhandle = targs_handle_init(max);

#if defined(USE_CHECKPOINT) && USE_CHECKPOINT
	thhandle->progress->common_count = count;

	for (unsigned int i = 0; i < EVP_MAX_MD_SIZE; i++)
		thhandle->progress->common_md_value[i] = mdtmp[i];
#endif

	for (; lmax <= max;) {
		fprintf(stderr, "Checking range: [%llu, %llu]\n", lmin, lmax);
		taskboard_set(thhandle->progress, lmin, lmax);
		for (thread_t thread = 0; thread < THREADS; thread++)
			assert(pthread_create(&threads[thread], NULL, thread_worker, (void *)(thhandle->targs[thread])) == 0);
		for (thread_t thread = 0; thread < THREADS; thread++)
			pthread_join(threads[thread], 0);

		taskboard_print(thhandle->progress);
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