// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#if USE_CHECKPOINT
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>	// isdigit
#include "helper.h"
#include "taskboard.h"
#include "options.h"
#include "interval.h"

int touch_checkpoint(struct options_t options, struct interval_t interval)
{
	if (options.checkpoint == NULL)
		return 0;

	FILE *fp;
	fp = fopen(options.checkpoint, "r");
	if (fp != NULL) {
		fclose(fp);
		fprintf(stderr, "%s already exists\n", options.checkpoint);
		return 1;
	}
	fp = fopen(options.checkpoint, "w+");
	fprintf(fp, "%ju %ju\n", (uintmax_t)(interval.min), (uintmax_t)(interval.max));
	fclose(fp);
	return 0;
}

static void err_baditem(char* filename, vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %ju item #%ju has bad data:\n", filename, (uintmax_t)line, (uintmax_t)item);
}

static void err_conflict(char* filename, vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %ju item #%ju has conflicting data:\n", filename, (uintmax_t)line, (uintmax_t)item);
}

static void err_unexpected_char(char* filename, int ch, vamp_t line, vamp_t item)
{
	err_baditem(filename,line, item);
	fprintf(stderr, "Unexpected ");
	switch (ch) {
		case ' ':
			fprintf(stderr, "space character.\n");
			break;
		case '\t':
			fprintf(stderr, "tab character.\n");
			break;
		case '\n':
			fprintf(stderr, "newline character.\n");
			break;
		default:
			fprintf(stderr, "character: ");
			if (isgraph(ch))
				fprintf(stderr, "%c\n", ch);
			else
				fprintf(stderr, "'%c'\n", ch);
			break;
	}
}

static int concat_digit(char *filename, vamp_t *number, int ch, vamp_t line, vamp_t item)
{
	int rc = 0;

	if (!isdigit(ch)) {
		err_unexpected_char(filename, ch, line, item);
		rc = 1;
		goto out;
	}
	digit_t digit = ch - '0';

	if (willoverflow(*number, VAMP_MAX, digit)) {
		err_baditem(filename, line, item);
		fprintf(stderr, "Out of interval: [0, %ju]\n", (uintmax_t)VAMP_MAX);
		rc = 1;
		goto out;
	}
	*number = 10 * (*number) + digit;

out:
	return rc;
}

#if (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH)
static int hash_set(char *filename, struct hash *ptr, int ch, int hash_index, vamp_t line, vamp_t item)
{
	int rc = 0;
	if (hash_index == ptr->md_size * 2) {
		err_unexpected_char(filename, ch, line , item);
		fprintf(stderr, "The checksum character length is invalid; Too many characters or missing newline.\n");
		rc = 1;
		goto out;
	}
	if (isspace(ch)) {
		err_unexpected_char(filename, ch, line , item);
		fprintf(stderr, "The checksum character length is invalid; Too few characters.\n");
		rc = 1;
		goto out;
	}
	if (!isxdigit(ch)) {
		err_unexpected_char(filename, ch, line, item);
		rc = 1;
		goto out;
	}

	int hi, lo; // nibbles
	{
		char str[2] = {ch, '\0'};
		int nibble = strtoul(str, NULL, 16);
		if (hash_index % 2 == 0) {
			hi = nibble;
			lo = ptr->md_value[hash_index/2] & 0x0f;
		} else {
			hi = (ptr->md_value[hash_index/2] >> 4) & 0x0f;
			lo = nibble;
		}
	}
	ptr->md_value[hash_index/2] = (hi << 4) | lo;
out:
	return rc;
}
static char count_end()
{
	return (' ');
}
#else /* (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH) */
static int hash_set(
	__attribute__((unused)) char *filename,
	__attribute__((unused)) struct hash *ptr,
	__attribute__((unused)) int ch,
	__attribute__((unused)) int hash_index,
	__attribute__((unused)) vamp_t line,
	__attribute__((unused)) vamp_t item)
{
	return 0;
}
static char count_end()
{
	return ('\n');
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH) */

int load_checkpoint(struct options_t options, struct interval_t *interval, struct taskboard *progress)
{
	assert(progress != NULL);
	if (options.checkpoint == NULL)
		return 0;

	FILE *fp = fopen(options.checkpoint, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s doesn't exist\n", options.checkpoint);
		return 1;
	}

	int rc = 0;

	enum types {integer, hash};
	enum names {min, max, complete, count, checksum = count + FANG_PAIRS_SIZE};

	char end_char[4 + FANG_PAIRS_SIZE] = {' ', '\n', ' '};

	// pls compiler no complain
	volatile size_t fang_pairs_size = FANG_PAIRS_SIZE;
	for (size_t i = 0; i < fang_pairs_size - 1; i++)
		end_char[3 + i] = ' ';
	end_char[2 + FANG_PAIRS_SIZE] = count_end();
	end_char[3 + FANG_PAIRS_SIZE] = '\n';

	int type[4 + FANG_PAIRS_SIZE] = {integer, integer, integer};
	for (size_t i = 0; i < FANG_PAIRS_SIZE; i++)
		type[3 + i] = integer;
	type[3 + FANG_PAIRS_SIZE] = hash;

	int name = min;
	vamp_t line = 1;
	vamp_t item = 1;

	bool is_empty = true;
	vamp_t num = 0;
	int hash_index = 0;

	while (!rc) {
		int ch = fgetc(fp);

		if (feof(fp)) {
			if (name != complete || !is_empty) {
				err_baditem(options.checkpoint, line, item);
				fprintf(stderr, "Unexpected end of file or missing newline.\n");
				rc = 1;
			}
			break;
		}
		if (ferror(fp)) {
			err_baditem(options.checkpoint, line, item);
			fprintf(stderr, "Unexpected end of file, caused by I/O error.\n");
			rc = 1;
			break;
		}

		if (ch == end_char[name]) {
			switch (name) {
				case min:
					options.min = num;
					options.max = num;
					rc = interval_set(interval, options);
					break;

				case max:
					if (num < interval->min) {
						err_conflict(options.checkpoint, line, item);
						fprintf(stderr, "max < min\n");
						rc = 1;
					} else {
						vamp_t tmp = interval->min;
						options.min = tmp;
						options.max = num;
						rc = interval_set(interval, options);
					}
					break;

				case complete:
					if (num < interval->min) {
						err_conflict(options.checkpoint, line, item);
						fprintf(stderr, "%ju < %ju (below min)\n", (uintmax_t)num, (uintmax_t)(interval->min));
						rc = 1;
					}
					else if (num > interval->max) {
						err_conflict(options.checkpoint, line, item);
						fprintf(stderr, "%ju > %ju (above max)\n", (uintmax_t)num, (uintmax_t)(interval->max));
						rc = 1;
					}
					else if (num <= interval->complete && line != 2) {
						err_conflict(options.checkpoint, line, item);
						fprintf(stderr, "%ju <= %ju (below previous)\n", (uintmax_t)num, (uintmax_t)(interval->complete));
						rc = 1;
					} else {
						rc = interval_set_complete(interval, num);
					}
					break;

				default:
					if (name < count || name > checksum)
						break;
					// Allow count+1, count+2, ... count+n to fall through
					__attribute__((fallthrough));
				case count: {
					bool not_first_column = (name > count);

					/*
					 * (previous line): ... [    ] [prev]
					 * (current  line): ... [left] [now ]
					 */
					
					size_t prev = name - count;
					size_t left = 0;
					if (not_first_column)
						left = name - count -1;
					vamp_t *element_prev = &(progress->common_count[prev]);
					vamp_t *element_left = NULL;
					if (not_first_column)
						element_left = &(progress->common_count[left]);

					if ((num < *element_prev) && (line != 2)) {
						err_conflict(options.checkpoint, line, item);
						fprintf(stderr, "%ju < %ju (below previous)\n", (uintmax_t)num, (uintmax_t)(*element_prev));
						rc = 1;
					}
					else if (not_first_column && (num > *element_left)) {
						err_conflict(options.checkpoint, line, item);
						size_t left_pairs = left + MIN_FANG_PAIRS;
						size_t pairs      = left_pairs + 1;
						fprintf(stderr, "%ju > %ju (More vampire numbers with %zu pairs than %zu pairs)\n", (uintmax_t)num, (uintmax_t)(*element_left), pairs, left_pairs);
						rc = 1;
					}
#if VAMPIRE_NUMBER_OUTPUTS
					else if ((num > 0) && (num - 1 > interval->complete - interval->min)) {
						err_conflict(options.checkpoint, line, item);
						fprintf(stderr, "More vampire numbers than numbers.\n");
						rc = 1;
					}
#endif
					(*element_prev) = num;
					break;
				}
#if (VAMPIRE_NUMBER_OUTPUTS) &&  (VAMPIRE_HASH)
				case checksum:
					if (hash_index < progress->checksum->md_size * 2) {
						err_unexpected_char(options.checkpoint, ch, line , item);
						fprintf(stderr, "The checksum character length is invalid; Too few characters.\n");
						rc = 1;
					}
					break;
#endif
			}
			num = 0;
			hash_index = 0;
			is_empty = true;
			name++;
			item++;
		} else {
			switch(type[name]) {
				case integer:
					rc = concat_digit(options.checkpoint, &num, ch, line, item);
					is_empty = false;
					break;

				case hash:
					rc = hash_set(options.checkpoint, progress->checksum, ch, hash_index++, line, item);
					is_empty = false;
					break;
			}
		}
		if (ch == '\n') {
			name = complete;
			line++;
			item = 1;
		}
	}
	fclose(fp);
	return rc;
}

void save_checkpoint(struct options_t options, vamp_t complete, struct taskboard *progress)
{
	if (options.checkpoint == NULL)
		return;

	FILE *fp = fopen(options.checkpoint, "a");
	assert(fp != NULL);

	fprintf(fp, "%ju", (uintmax_t)complete);
	for (size_t i = 0; i < FANG_PAIRS_SIZE; i++)
		fprintf(fp, " %ju", (uintmax_t)(progress->common_count[i]));


#if (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH)
	fprintf(fp, " ");
	for (int i = 0; i < progress->checksum->md_size; i++)
		fprintf(fp, "%02x", progress->checksum->md_value[i]);
#endif

	fprintf(fp, "\n");
	fclose(fp);
}

#endif /* USE_CHECKPOINT */
