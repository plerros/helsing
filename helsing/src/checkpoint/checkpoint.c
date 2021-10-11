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
	fprintf(fp, "%llu %llu\n", interval.min, interval.max);
	fclose(fp);
	return 0;
}

static void err_baditem(vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %llu item #%llu has bad data:\n", CHECKPOINT_FILE, line, item);
}

static void err_conflict(vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %llu item #%llu has conflicting data:\n", CHECKPOINT_FILE, line, item);
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

	if (willoverflow(*number, vamp_max, digit)) {
		err_baditem(line, item);
		fprintf(stderr, "Out of interval: [0, %llu]\n", vamp_max);
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
	enum names {min, max, complete, count, checksum};

	char end_char[5] = {' ', '\n', ' ', count_end(), '\n'};
	int type[5] = {integer, integer, integer, integer, hash};

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
					rc = interval_set(interval, num, num);
					break;

				case max:
					if (num < interval->min) {
						err_conflict(line, item);
						fprintf(stderr, "max < min\n");
						rc = 1;
					} else {
						vamp_t tmp = interval->min;
						rc = interval_set(interval, tmp, num);
					}
					break;

				case complete:
					if (num < interval->min) {
						err_conflict(line, item);
						fprintf(stderr, "%llu < %llu (below min)\n", num, interval->min);
						rc = 1;
					}
					else if (num > interval->max) {
						err_conflict(line, item);
						fprintf(stderr, "%llu > %llu (above max)\n", num, interval->max);
						rc = 1;
					}
					else if (num <= interval->complete && line != 2) {
						err_conflict(line, item);
						fprintf(stderr, "%llu <= %llu (below previous)\n", num, interval->complete);
						rc = 1;
					} else {
						rc = interval_set_complete(interval, num);
					}
					break;

				case count:
					if (num < progress->common_count && line != 2) {
						err_conflict(line, item);
						fprintf(stderr, "%llu < %llu (below previous)\n", num, progress->common_count);
						rc = 1;
					}
#ifdef PROCESS_RESULTS
					else if (num > 0 && num - 1 > interval->complete - interval->min) {
						err_conflict(line, item);
						fprintf(stderr, "More vampire numbers than numbers.\n");
						rc = 1;
					}
#endif /* PROCESS_RESULTS */
					progress->common_count = num;
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
					break;

				case hash:
					rc = hash_set(progress->checksum, ch, hash_index++, line, item);
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

	fprintf(fp, "%llu %llu", complete, progress->common_count);

#ifdef CHECKSUM_RESULTS
	fprintf(fp, " ");
	for (int i = 0; i < progress->checksum->md_size; i++)
		fprintf(fp, "%02x", progress->checksum->md_value[i]);
#endif /* CHECKSUM_RESULTS */

	fprintf(fp, "\n");
	fclose(fp);
}

#endif /* USE_CHECKPOINT */
