// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
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

	printf("    FANG_PAIR_OUTPUTS=%s\n", (FANG_PAIR_OUTPUTS ? "true" : "false"));
	if (FANG_PAIR_OUTPUTS) {
		printf("        FANG_PRINT=%s\n", (FANG_PRINT ? "true" : "false"));
	}
	printf("    VAMPIRE_NUMBER_OUTPUTS=%s\n", (VAMPIRE_NUMBER_OUTPUTS ? "true" : "false"));
	if (VAMPIRE_NUMBER_OUTPUTS) {
		printf("        VAMPIRE_INDEX=%s\n", (VAMPIRE_INDEX ? "true" : "false"));
		printf("        VAMPIRE_PRINT=%s\n", (VAMPIRE_PRINT ? "true" : "false"));
		printf("        VAMPIRE_INTEGRAL=%s\n", (VAMPIRE_INTEGRAL ? "true" : "false"));
		printf("        VAMPIRE_HASH=%s\n", (VAMPIRE_HASH ? "true" : "false"));
		if (VAMPIRE_HASH)
			printf("    DIGEST_NAME=%s\n", DIGEST_NAME);

		printf("    MIN_FANG_PAIRS=%d\n", MIN_FANG_PAIRS);
		printf("    MAX_FANG_PAIRS=%d\n", MAX_FANG_PAIRS);
	}
	printf("    MEASURE_RUNTIME=%s\n", (MEASURE_RUNTIME ? "true" : "false"));
	printf("    ALG_NORMAL=%s\n", (ALG_NORMAL ? "true" : "false"));
	printf("    ALG_CACHE=%s\n", (ALG_CACHE ? "true" : "false"));
	if (ALG_CACHE) {
		printf("        PARTITION_METHOD=%d\n", PARTITION_METHOD);
		printf("        MULTIPLICAND_PARTITIONS=%d\n", MULTIPLICAND_PARTITIONS);
		printf("        PRODUCT_PARTITIONS=%d\n", PRODUCT_PARTITIONS);
	}
	printf("    BASE=%d\n", BASE);
	printf("    MAX_TASK_SIZE=%ju\n", (uintmax_t)MAX_TASK_SIZE);
	printf("    USE_CHECKPOINT=%s\n", (USE_CHECKPOINT ? "true" : "false"));
	printf("    LINK_SIZE=%d\n", LINK_SIZE);
	printf("    SAFETY_CHECKS=%s\n", (SAFETY_CHECKS ? "true" : "false"));
	printf("    THREADS_PTHREAD=%s\n", (THREADS_PTHREAD ? "true" : "false"));
}

static void arg_checkpoint()
{
#if USE_CHECKPOINT
	printf("  -c [checkpoint]  continue from checkpoint\n");
#endif
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
	arg_checkpoint();
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
		fprintf(stderr, "Input out of range: [%ju, %ju]\n", (uintmax_t)min, (uintmax_t)max);
	return err;
}

int options_new(struct options_t **ptr, int argc, char *argv[])
{
	struct options_t *new = malloc(sizeof(struct options_t));
	if (new == NULL)
		abort();

	new->threads = 1;
	new->manual_task_size = 0;
	new->display_progress = false;
	new->dry_run = false;
	new->min = 0;
	new->max = 0;
	new->checkpoint = NULL;

#if (defined _SC_NPROCESSORS_ONLN) && (THREADS_PTHREAD)
	new->threads = sysconf(_SC_NPROCESSORS_ONLN);
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
			{"checkpoint", required_argument, NULL, 'c'},
			{"dry-run", no_argument, &dry_run, 1},
			{"lower bound", required_argument, NULL, 'l'},
			{"n digits", required_argument, NULL, 'n'},
			{"manual task size", required_argument, NULL, 's'},
			{"threads", required_argument, NULL, 't'},
			{"upper bound", required_argument, NULL, 'u'},
			{NULL, 0, NULL, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "c:l:n:s:t:u:", long_options, &option_index);

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

			case 'c':
				if (new->checkpoint != NULL) {
					help();
					rc = 1;
				} else {
					size_t len = strlen(optarg) + 1;
					new->checkpoint = malloc(len);
					if (new->checkpoint == NULL)
						abort();
					strcpy(new->checkpoint, optarg);
				}
				break;

			case 'l':
				if (min_is_set) {
					help();
					rc = 1;
				} else {
					rc = strtov(optarg, 0, VAMP_MAX, &(new->min));
					min_is_set = true;
				}
				break;
			case 'n':
				if (min_is_set || max_is_set) {
					help();
					rc = 1;
				} else {
					vamp_t tmp;
					rc = strtov(optarg, 1, length(VAMP_MAX), &tmp);
					if (rc)
						break;
					new->min = pow_v(tmp - 1);
					new->max = (new->min - 1) * BASE + (BASE - 1); // avoid overflow
					min_is_set = true;
					max_is_set = true;
				}
				break;
			case 's':
				if (new->manual_task_size != 0) {
					help();
					rc = 1;
				} else {
					vamp_t tmp;
					rc = strtov(optarg, 1, VAMP_MAX, &tmp);
					if (rc)
						break;
					new->manual_task_size = tmp;
				}
				break;
			case 't':
				{
					vamp_t tmp;
					rc = strtov(optarg, 1, THREAD_T_MAX, &tmp);
					if (rc)
						break;
					new->threads = tmp;
				}
				break;
			case 'u':
				if (max_is_set) {
					help();
					rc = 1;
				} else {
					rc = strtov(optarg, 0, VAMP_MAX, &(new->max));
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

	if (!min_is_set && !max_is_set) {
		char buffer[100];
	
		printf("Lower bound: ");
		fgets(buffer, sizeof(buffer), stdin);
		rc = strtov(buffer, 0, VAMP_MAX, &(new->min));
		if (rc)
			goto out;
		min_is_set = true;

		printf("Upper bound: ");
		fgets(buffer, sizeof(buffer), stdin);
		rc = strtov(buffer, 0, VAMP_MAX, &(new->max));
		if (rc)
			goto out;

		max_is_set = true;
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

	if ((!min_is_set) && (!max_is_set) && (new->checkpoint == NULL)) {
		help();
		rc = 1;
		goto out;
	}

	if (display_progress)
		new->display_progress = true;
	if (dry_run)
		new->dry_run = true;
out:
	(*ptr) = new;
	return rc;
}

void options_free(struct options_t *ptr)
{
	if (ptr == NULL)
		return;
	
	free(ptr->checkpoint);
	free(ptr);
}

bool options_touch_checkpoint(struct options_t options)
{
	if (options.min == 0 && options.max == 0)
		return false;

	return true;
}
