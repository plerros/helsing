// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
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
#endif

#if USE_CHECKPOINT

int touch_checkpoint(struct options_t options, struct interval_t interval)
{
	if (options.load_checkpoint)
		return 0;

	FILE *fp;
	fp = fopen(CHECKPOINT_FILE, "r");
	if (fp != NULL) {
		fclose(fp);
		fprintf(stderr, "%s already exists\n", CHECKPOINT_FILE);
		return 1;
	}
	fp = fopen(CHECKPOINT_FILE, "w+");
	fprintf(fp, "%ju %ju\n", (uintmax_t)(interval.min), (uintmax_t)(interval.max));
	fclose(fp);
	return 0;
}

static void err_baditem(vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %ju item #%ju has bad data:\n", CHECKPOINT_FILE, (uintmax_t)line, (uintmax_t)item);
}

static void err_conflict(vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %ju item #%ju has conflicting data:\n", CHECKPOINT_FILE, (uintmax_t)line, (uintmax_t)item);
}

static void err_unexpected_char(int ch, vamp_t line, vamp_t item)
{
	err_baditem(line, item);
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

static int concat_digit(vamp_t *number, int ch, vamp_t line, vamp_t item)
{
	int rc = 0;

	if (!isdigit(ch)) {
		err_unexpected_char(ch, line, item);
		rc = 1;
		goto out;
	}
	digit_t digit = ch - '0';

	if (willoverflow(*number, VAMP_MAX, digit)) {
		err_baditem(line, item);
		fprintf(stderr, "Out of interval: [0, %ju]\n", (uintmax_t)VAMP_MAX);
		rc = 1;
		goto out;
	}
	*number = 10 * (*number) + digit;

out:
	return rc;
}

#ifdef CHECKSUM_RESULTS
static int hash_set(struct hash *ptr, int ch, int hash_index, vamp_t line, vamp_t item)
{
	int rc = 0;
	if (hash_index == ptr->md_size * 2) {
		err_unexpected_char(ch, line , item);
		fprintf(stderr, "The checksum character length is invalid; Too many characters or missing newline.\n");
		rc = 1;
		goto out;
	}
	if (isspace(ch)) {
		err_unexpected_char(ch, line , item);
		fprintf(stderr, "The checksum character length is invalid; Too few characters.\n");
		rc = 1;
		goto out;
	}
	if (!isxdigit(ch)) {
		err_unexpected_char(ch, line, item);
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
#else
static int hash_set(
	__attribute__((unused)) struct hash *ptr,
	__attribute__((unused)) int ch,
	__attribute__((unused)) int hash_index,
	__attribute__((unused)) vamp_t line,
	__attribute__((unused)) vamp_t item)
{
	return 0;
}
#endif

static char count_end()
{
#ifdef CHECKSUM_RESULTS
	return (' ');
#else
	return ('\n');
#endif
}

int load_checkpoint(struct interval_t *interval, struct taskboard *progress)
{
	assert(progress != NULL);

	FILE *fp = fopen(CHECKPOINT_FILE, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s doesn't exist\n", CHECKPOINT_FILE);
		return 1;
	}

	int rc = 0;

	enum types {integer, hash};
	enum names {min, max, complete, count, checksum = count + MAX_FANG_PAIRS};

	char end_char[4 + MAX_FANG_PAIRS] = {' ', '\n', ' '};
	for (size_t i = 0; i < MAX_FANG_PAIRS - 1; i++)
		end_char[3 + i] = ' ';
	end_char[2 + MAX_FANG_PAIRS] = count_end();
	end_char[3 + MAX_FANG_PAIRS] = '\n';

	int type[4 + MAX_FANG_PAIRS] = {integer, integer, integer};
	for (size_t i = 0; i < MAX_FANG_PAIRS; i++)
		type[3 + i] = integer;
	type[3 + MAX_FANG_PAIRS] = hash;

	int name = min;
	vamp_t line = 1;
	vamp_t item = 1;

	bool is_empty = true;
	vamp_t num = 0;
	int hash_index = 0;
	struct options_t options;

	while (!rc) {
		int ch = fgetc(fp);

		if (feof(fp)) {
			if (name != complete || !is_empty) {
				err_baditem(line, item);
				fprintf(stderr, "Unexpected end of file or missing newline.\n");
				rc = 1;
			}
			break;
		}
		if (ferror(fp)) {
			err_baditem(line, item);
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
						err_conflict(line, item);
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
						err_conflict(line, item);
						fprintf(stderr, "%ju < %ju (below min)\n", (uintmax_t)num, (uintmax_t)(interval->min));
						rc = 1;
					}
					else if (num > interval->max) {
						err_conflict(line, item);
						fprintf(stderr, "%ju > %ju (above max)\n", (uintmax_t)num, (uintmax_t)(interval->max));
						rc = 1;
					}
					else if (num <= interval->complete && line != 2) {
						err_conflict(line, item);
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
				case count:
					if (num < progress->common_count[name - count] && line != 2) {
						err_conflict(line, item);
						fprintf(stderr, "%ju < %ju (below previous)\n", num, (uintmax_t)(progress->common_count[name - count]));
						rc = 1;
					}
#ifdef PROCESS_RESULTS
					else if (num > 0 && num - 1 > interval->complete - interval->min) {
						err_conflict(line, item);
						fprintf(stderr, "More vampire numbers than numbers.\n");
						rc = 1;
					}
#endif /* PROCESS_RESULTS */
					progress->common_count[name - count] = num;
					break;
#ifdef CHECKSUM_RESULTS
				case checksum:
					if (hash_index < progress->checksum->md_size * 2) {
						err_unexpected_char(ch, line , item);
						fprintf(stderr, "The checksum character length is invalid; Too few characters.\n");
						rc = 1;
					}
					break;
#endif /* CHECKSUM_RESULTS */
			}
			num = 0;
			hash_index = 0;
			is_empty = true;
			name++;
			item++;
		} else {
			switch(type[name]) {
				case integer:
					rc = concat_digit(&num, ch, line, item);
					is_empty = false;
					break;

				case hash:
					rc = hash_set(progress->checksum, ch, hash_index++, line, item);
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

void save_checkpoint(vamp_t complete, struct taskboard *progress)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "a");
	assert(fp != NULL);

	fprintf(fp, "%ju", (uintmax_t)complete);
	for (size_t i = 0; i < MAX_FANG_PAIRS; i++)
		fprintf(fp, " %ju", (uintmax_t)(progress->common_count[i]));


#ifdef CHECKSUM_RESULTS
	fprintf(fp, " ");
	for (int i = 0; i < progress->checksum->md_size; i++)
		fprintf(fp, "%02x", progress->checksum->md_value[i]);
#endif /* CHECKSUM_RESULTS */

	fprintf(fp, "\n");
	fclose(fp);
}

#endif /* USE_CHECKPOINT */
