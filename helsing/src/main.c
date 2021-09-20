// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>	// isdigit
#include <getopt.h>
#include <unistd.h>

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

static void argument_lower_bound()
{
	printf("  -l [min]   interval lower bound, requires: -u\n");
}

static void argument_upper_bound()
{
	printf("  -u [max]   interval upper bound, requires: -l\n");
}

static void help()
{
	printf("Usage: helsing [options] [interval options]\n");
	printf("\nOptions:\n");
	printf("    --help   show help\n");
	printf("\nInterval options:\n");
#if USE_CHECKPOINT
	printf("  (empty)    recover from checkpoint\n");
#endif
	argument_lower_bound();
	argument_upper_bound();
}

int main(int argc, char *argv[])
{
	struct interval_t interval;
	struct taskboard *progress = NULL;

	vamp_t min, max;
	int err = 0;
	static int help_flag = 0;
	bool min_is_set = false;
	bool max_is_set = false;

	int c;
	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, &help_flag, 1},
			{"lower bound", required_argument, NULL, 'l'},
			{"upper bound", required_argument, NULL, 'u'},
			{NULL, 0, NULL, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "l:u:", long_options, &option_index);

		// Detect end of the options
		if (c == -1)
			break;

		if (help_flag) {
			help();
			goto out;
		}

		switch (c) {
			case 0:
				abort();

			case 'l':
				err = strtov(optarg, &min);
				if (err)
					fprintf(stderr, "Input out of range: [0, %llu]\n", vamp_max);
				else
					min_is_set = true;
				break;
			case 'u':
				err = strtov(optarg, &max);
				if (err)
					fprintf(stderr, "Input out of range: [0, %llu]\n", vamp_max);
				else
					max_is_set = true;
				break;
			case '?':
				err = 1;
				break;

			default:
				abort();
		}
		if (err)
			goto out;
	}
	if (optind < argc) {
		printf ("non-option ARGV-elements: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);

		putchar ('\n');
		goto out;
	}

	if (min_is_set ^ max_is_set) {
		printf("Missing argument:\n");
		if (max_is_set)
			argument_lower_bound();
		else
			argument_upper_bound();
		goto out;
	}
	if (!min_is_set && !max_is_set && !USE_CHECKPOINT) {
		help();
		goto out;
	}

	if (min_is_set && max_is_set) {
		if (interval_set(&interval, min, max))
			goto out;
		if (touch_checkpoint(interval))
			goto out;
	}

	taskboard_new(&progress);

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

		interval.complete = lmax;
	}
	targs_handle_print(thhandle);
	targs_handle_free(thhandle);
out:
	taskboard_free(progress);
	return 0;
}
