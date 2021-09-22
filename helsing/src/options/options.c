// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h> // isdigit
#include <getopt.h>
#include <unistd.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "options.h"
#include "helper.h"

static void arg_lower_bound()
{
	printf("  -l [min]         set interval lower bound\n");
}

static void arg_number_of_digits()
{
	printf("  -n [n digits]    set interval to [%u^(n - 1), %u^n - 1]\n", BASE, BASE);
}

static void arg_manual_task_size()
{
	printf("  -s [task size]   set task size\n");
}

static void arg_threads()
{
	printf("  -t [threads]     set # of threads\n");
}

static void arg_upper_bound()
{
	printf("  -u [max]         set interval upper bound\n");
}

static void help()
{
	printf("Usage: helsing [options] [interval options]\n");
	printf("\nOptions:\n");
	printf("    --help         show help\n");
	arg_manual_task_size();
	arg_threads();
	printf("\nInterval options:\n");
#if USE_CHECKPOINT
	printf("  (empty)          recover from checkpoint\n");
#endif
	arg_lower_bound();
	arg_upper_bound();
	arg_number_of_digits();
}

static int strtov(const char *str, vamp_t min, vamp_t max, vamp_t *number) // string to vamp_t
{
	assert(str != NULL);
	assert(number != NULL);
	int err = 0;
	vamp_t tmp = 0;
	for (length_t i = 0; isgraph(str[i]); i++) {
		if (!isdigit(str[i])) {
			err = 1;
			goto out;
		}
		digit_t digit = str[i] - '0';
		if (willoverflow(tmp, max, digit)) {
			err = 1;
			goto out;
		}
		tmp = 10 * tmp + digit;
	}
	if (tmp < min) {
		err = 1;
		goto out;
	}
	*number = tmp;
out:
	if (err)
		fprintf(stderr, "Input out of range: [%llu, %llu]\n", min, max);
	return err;
}

static length_t get_max_length()
{
	length_t ret = 0;
	for (vamp_t i = vamp_max; i >= BASE - 1; i /= BASE)
		ret ++;
	return ret;
}

int options_init(struct options_t* ptr, int argc, char *argv[], vamp_t *min, vamp_t *max)
{
	ptr->threads = 1;
	ptr->manual_task_size = 0;

#ifdef _SC_NPROCESSORS_ONLN
	ptr->threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif

	int rc = 0;
	static int help_flag = 0;
	bool min_is_set = false;
	bool max_is_set = false;

	int c;
	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, &help_flag, 1},
			{"lower bound", required_argument, NULL, 'l'},
			{"n digits", required_argument, NULL, 'n'},
			{"manual task size", required_argument, NULL, 's'},
			{"threads", required_argument, NULL, 't'},
			{"upper bound", required_argument, NULL, 'u'},
			{NULL, 0, NULL, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "l:n:s:t:u:", long_options, &option_index);

		// Detect end of the options
		if (c == -1)
			break;

		if (help_flag) {
			help();
			rc = 1;
			goto out;
		}

		switch (c) {
			case 0:
				break;

			case 'l':
				if (min_is_set) {
					help();
					rc = 1;
				} else {
					rc = strtov(optarg, 0, vamp_max, min);					
					min_is_set = true;
				}
				break;
			case 'n':
				if (min_is_set || max_is_set) {
					help();
					rc = 1;
				} else {
					vamp_t tmp;
					rc = strtov(optarg, 1, get_max_length(), &tmp);
					if (rc)
						break;
					*min = pow_v(tmp - 1);
					*max = (*min - 1) * BASE + (BASE - 1); // avoid overflow
					min_is_set = true;
					max_is_set = true;
				}
				break;
			case 's':
				if (ptr->manual_task_size != 0) {
					help();
					rc = 1;
				} else {
					vamp_t tmp;
					rc = strtov(optarg, 1, vamp_max, &tmp);
					if (rc)
						break;
					ptr->manual_task_size = tmp;
				}
				break;
			case 't':
				{
					vamp_t tmp;
					rc = strtov(optarg, 1, thread_max, &tmp);
					if (rc)
						break;
					ptr->threads = tmp;
				}
				break;
			case 'u':
				if (max_is_set) {
					help();
					rc = 1;
				} else {
					rc = strtov(optarg, 0, vamp_max, max);
					max_is_set = true;
				}
				break;
			case '?':
				rc = 1;
				break;

			default:
				abort();
		}
		if (rc)
			goto out;
	}


	if (optind < argc) {
		printf ("non-option ARGV-elements: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);

		putchar ('\n');
		rc = 1;
		goto out;
	}

	if (min_is_set ^ max_is_set) {
		printf("Missing argument:\n");
		if (max_is_set)
			arg_lower_bound();
		else
			arg_upper_bound();
		rc = 1;
		goto out;
	}
	if (!min_is_set && !max_is_set && !USE_CHECKPOINT) {
		help();
		rc = 1;
		goto out;
	}
out:
	return rc;
}
