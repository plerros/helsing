// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2022 Pierro Zachareas
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

static void buildconf()
{
	printf("  configuration:\n");
	printf("    VERBOSE_LEVEL=%d\n", VERBOSE_LEVEL);
	if (VERBOSE_LEVEL == 3)
		printf("    DIGEST_NAME=%s\n", DIGEST_NAME);
	if (VERBOSE_LEVEL > 1)
		printf("    MIN_FANG_PAIRS=%d\n", MIN_FANG_PAIRS);
	printf("    MEASURE_RUNTIME=%s\n", (MEASURE_RUNTIME ? "true" : "false"));
	printf("    ALG_NORMAL=%s\n", (ALG_NORMAL ? "true" : "false"));
	printf("    ALG_CACHE=%s\n", (ALG_CACHE ? "true" : "false"));
	if (ALG_CACHE) {
		printf("    COMPARISON_BITS=%d\n", COMPARISON_BITS);
		printf("    PARTITION_METHOD=%d\n", PARTITION_METHOD);
	}
	printf("    BASE=%d\n", BASE);
	printf("    MAX_TASK_SIZE=%llu\n", MAX_TASK_SIZE);
	printf("    USE_CHECKPOINT=%s\n", (USE_CHECKPOINT ? "true" : "false"));
	if (USE_CHECKPOINT)
		printf("    CHECKPOINT_FILE=%s\n", CHECKPOINT_FILE);
	printf("    LINK_SIZE=%d\n", LINK_SIZE);
	printf("    SAFETY_CHECKS=%s\n", (SAFETY_CHECKS ? "true" : "false"));
}

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
	printf("Scan a given interval for vampire numbers.\n");
	printf("\nOptions:\n");
	printf("    --buildconf    show build configuration\n");
	printf("    --help         show help\n");
	printf("    --progress     display progress\n");
	printf("    --dry-run      perform a trial run without any calculations\n");
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
	for (vamp_t i = VAMP_MAX; i >= BASE - 1; i /= BASE)
		ret ++;
	return ret;
}

int options_init(struct options_t* ptr, int argc, char *argv[], vamp_t *min, vamp_t *max)
{
	ptr->threads = 1;
	ptr->manual_task_size = 0;
	ptr->display_progress = false;
	ptr->dry_run = false;
	ptr->load_checkpoint = false;

#ifdef _SC_NPROCESSORS_ONLN
	ptr->threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif

	int rc = 0;
	static int buildconf_flag = 0;
	static int help_flag = 0;
	static int display_progress = 0;
	static int dry_run = 0;
	bool min_is_set = false;
	bool max_is_set = false;

	int c;
	while (1) {
		static struct option long_options[] = {
			{"buildconf", no_argument, &buildconf_flag, 1},
			{"help", no_argument, &help_flag, 1},
			{"progress", no_argument, &display_progress, 1},
			{"dry-run", no_argument, &dry_run, 1},
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

		if (buildconf_flag) {
			buildconf();
			rc = 1;
			goto out;
		}
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
					rc = strtov(optarg, 0, VAMP_MAX, min);
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
					rc = strtov(optarg, 1, VAMP_MAX, &tmp);
					if (rc)
						break;
					ptr->manual_task_size = tmp;
				}
				break;
			case 't':
				{
					vamp_t tmp;
					rc = strtov(optarg, 1, THREAD_MAX, &tmp);
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
					rc = strtov(optarg, 0, VAMP_MAX, max);
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
	if (!min_is_set && !max_is_set) {
		if (USE_CHECKPOINT) {
			ptr->load_checkpoint = true;
		} else {
			help();
			rc = 1;
			goto out;
		}
	}
	if (display_progress)
		ptr->display_progress = true;
	if (dry_run)
		ptr->dry_run = true;
out:
	return rc;
}
