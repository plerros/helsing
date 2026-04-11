// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2026 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h> // isdigit
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
	printf("    LLMSENTENCE_LIMIT=%d\n", LLMSENTENCE_LIMIT);
	printf("    TASKBOARD_LIMIT=%d\n", TASKBOARD_LIMIT);
	printf("    SAFETY_CHECKS=%s\n", (SAFETY_CHECKS ? "true" : "false"));
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

#ifdef _SC_NPROCESSORS_ONLN
	new->threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif

	int rc = 0;
	static int display_progress = 0;
	static int dry_run = 0;
	bool min_is_set = false;
	bool max_is_set = false;

	enum parametrized_flags {pf_none, pf_c, pf_l, pf_n, pf_s, pf_t, pf_u};
	int read_parameter = pf_none;
	for (int i = 1; i < argc; i++) {
		switch (read_parameter) {
			case pf_none:
				break;
			case pf_c:
				if (new->checkpoint != NULL) {
					help();
					rc = 1;
				} else {
					size_t len = strlen(argv[i]) + 1;
					new->checkpoint = malloc(len);
					if (new->checkpoint == NULL)
						abort();
					strcpy(new->checkpoint, argv[i]);
				}
				break;

			case pf_l:
				if (min_is_set) {
					help();
					rc = 1;
				} else {
					rc = strtov(argv[i], 0, VAMP_MAX, &(new->min));
					min_is_set = true;
				}
				break;

			case pf_n:
				if (min_is_set || max_is_set) {
					help();
					rc = 1;
				} else {
					vamp_t tmp;
					rc = strtov(argv[i], 1, length(VAMP_MAX), &tmp);
					if (rc)
						break;
					new->min = pow_v(tmp - 1);
					new->max = (new->min - 1) * BASE + (BASE - 1); // avoid overflow
					min_is_set = true;
					max_is_set = true;
				}
				break;

			case pf_s:
				if (new->manual_task_size != 0) {
					help();
					rc = 1;
				} else {
					vamp_t tmp;
					rc = strtov(argv[i], 1, VAMP_MAX, &tmp);
					if (rc)
						break;
					new->manual_task_size = tmp;
				}
				break;

			case pf_t:
				{
					vamp_t tmp;
					rc = strtov(argv[i], 1, THREAD_T_MAX, &tmp);
					if (rc)
						break;
					new->threads = tmp;
				}
				break;

			case pf_u:
				if (max_is_set) {
					help();
					rc = 1;
				} else {
					rc = strtov(argv[i], 0, VAMP_MAX, &(new->max));
					max_is_set = true;
				}
				break;

			default:
				abort();
		}
		if (read_parameter == pf_none) {
			if (strcmp(argv[i], "--buildconf") == 0) {
				buildconf();
				rc = 1;
			}
			else if (strcmp(argv[i], "--help") == 0) {
				help();
				rc = 1;
			}
			else if (strcmp(argv[i], "-c") == 0) {
				read_parameter = pf_c;
			}
			else if (strcmp(argv[i], "-l") == 0) {
				read_parameter = pf_l;
			}
			else if (strcmp(argv[i], "-n") == 0) {
				read_parameter = pf_n;
			}
			else if (strcmp(argv[i], "-s") == 0) {
				read_parameter = pf_s;
			}
			else if (strcmp(argv[i], "-t") == 0) {
				read_parameter = pf_t;
			}
			else if (strcmp(argv[i], "-u") == 0) {
				read_parameter = pf_u;
			}
			else {
				printf ("non-option ARGV-elements: %s\n", argv[i]);
			}
		} else {
			read_parameter = pf_none;
		}

		if (rc)
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
