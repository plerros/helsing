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
#endif

#if USE_CHECKPOINT
/*
 * ftov return values:
 *
 * -1: Empty
 * -2: Overflow
 */

static int ftov(FILE *fp, vamp_t *ptr, int *ch) // file to vamp_t
{
	assert(fp != NULL);
	assert(ptr != NULL);

	vamp_t number = 0;
	int ret = 0;
	bool is_empty = 1;
	while (1) {
		*ch = fgetc(fp);
		if (!isdigit(*ch))
			break;

		digit_t digit = *ch - '0';
		if (willoverflow(number, digit))
			return -2;
		number = 10 * number + digit;
		is_empty = 0;
	}
	if (is_empty)
		ret = -1;
	else
		*ptr = number;

	return ret;
}

#ifdef CHECKSUM_RESULTS

/*
 * ftomd return values:
 *
 * -1: Non hex character
 */

static int ftomd(FILE *fp, struct hash *ptr, int *ch)
{
	assert(fp != NULL);
	assert(ptr->md_value != NULL);

	for (int i = 0; i < ptr->md_size; i++) {
		char hex[3] = {0, 0, '\0'};
		for (int j = 0; j < 2; j++) {
			*ch = fgetc(fp);
			if (!isxdigit(*ch))
				return -1;
			hex[j] = *ch;
		}
		ptr->md_value[i] = strtoul(hex, NULL, 16);
	}
	*ch = fgetc(fp);
	return 0;
}
#endif /* CHECKSUM_RESULTS */

int touch_checkpoint(vamp_t min, vamp_t max)
{
	FILE *fp;
	fp = fopen(CHECKPOINT_FILE, "r");
	if (fp != NULL) {
		fclose(fp);
		fprintf(stderr, "%s already exists\n", CHECKPOINT_FILE);
		return 1;
	}
	fp = fopen(CHECKPOINT_FILE, "w+");
	fprintf(fp, "%llu %llu\n", min, max);
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

static void err_unexpected_char(FILE *fp, char ch, int line, int item)
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
		case EOF:
			if (feof(fp))
				fprintf(stderr, "end of file or missing newline.\n");
			if (ferror(fp))
				fprintf(stderr, "end of file, caused by I/O error.\n");
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

static void err_ftov(FILE *fp, int err, char ch, vamp_t line, vamp_t item)
{
	switch (err) {
		case -1:
			err_unexpected_char(fp, ch, line, item);
			break;
		case -2:
			err_baditem(line, item);
			fprintf(stderr, "Out of range: [0, %llu]\n", vamp_max);
			break;
	}
}

int load_checkpoint(vamp_t *min, vamp_t *max, vamp_t *current, struct taskboard *progress)
{
	assert(progress != NULL);

	FILE *fp = fopen(CHECKPOINT_FILE, "r");
	assert(fp != NULL);

	vamp_t line = 1;
	vamp_t item = 1;
	int ch = 0;
	int ret = 0;
	int err = 0;

	err = ftov(fp, min, &ch);
	if (err) {
		err_ftov(fp, err, ch, line , item);
		ret = 1;
		goto out;
	}
	else if (ch != ' ') {
		err_unexpected_char(fp, ch, line , item);
		ret = 1;
		goto out;
	}
	item++;

	err = ftov(fp, max, &ch);
	if (err) {
		err_ftov(fp, err, ch, line , item);
		ret = 1;
		goto out;
	}
	else if (ch != '\n') {
		err_unexpected_char(fp, ch, line , item);
		ret = 1;
		goto out;
	}

	if (*max < *min) {
		err_conflict(line, item);
		fprintf(stderr, "max < min\n");
		ret = 1;
		goto out;
	}

	*min = get_min(*min, *max);
	*max = get_min(*min, *max);

	if (cache_ovf_chk(*max)) {
		fprintf(stderr, "WARNING: the code might produce false positives, ");
		if (ELEMENT_BITS == 32)
			fprintf(stderr, "please set ELEMENT_BITS to 64.\n");
		else
			fprintf(stderr, "please set CACHE to false.\n");
		ret = 1;
	}

	progress->common_count = 0;
	line++;

	vamp_t prev = *current;
	vamp_t prevcount = 0;
	for (; !feof(fp); ) {
		item = 1;

		err = ftov(fp, current, &ch);
		if (err == -1 && ch == EOF && feof(fp))
			goto out;
		if (err) {
			err_ftov(fp, err, ch, line , item);
			ret = 1;
			goto out;
		}
		else if (ch != ' ') {
			err_unexpected_char(fp, ch, line , item);
			ret = 1;
			goto out;
		}

		if (*current < *min) {
			err_conflict(line, item);
			fprintf(stderr, "%llu < %llu (below min)\n", *current, *min);
			ret = 1;
			goto out;
		}
		if (*current > *max) {
			err_conflict(line, item);
			fprintf(stderr, "%llu > %llu (above max)\n", *current, *max);
			ret = 1;
			goto out;
		}
		if (*current <= prev && line != 2) {
			err_conflict(line, item);
			fprintf(stderr, "%llu <= %llu (below previous)\n", *current, prev);
			ret = 1;
			goto out;
		}
		item++;

		err = ftov(fp, &(progress->common_count), &ch);
		if (err) {
			err_ftov(fp, err, ch, line , item);
			ret = 1;
			goto out;
		}
#ifdef CHECKSUM_RESULTS
		else if (ch != ' ') {
#else /* CHECKSUM_RESULTS */
		else if (ch != '\n') {
#endif /* CHECKSUM_RESULTS */
			err_unexpected_char(fp, ch, line , item);
			ret = 1;
			goto out;
		}
		if (progress->common_count < prevcount && line != 2) {
			err_conflict(line, item);
			fprintf(stderr, "%llu < %llu (below previous)\n", progress->common_count, prevcount);
			ret = 1;
			goto out;
		}

#ifdef PROCESS_RESULTS
		if (progress->common_count > 0 && progress->common_count -1 > *current - *min) {
			err_conflict(line, item);
			fprintf(stderr, "More vampire numbers than numbers.\n");
			ret = 1;
			goto out;
		}
#endif /* PROCESS_RESULTS */

		item++;

#ifdef CHECKSUM_RESULTS
		err = ftomd(fp, progress->checksum, &ch);
		if (err) {
			err_unexpected_char(fp, ch, line , item);
			if (isspace(ch))
				fprintf(stderr, "The checksum character length is invalid; Too few characters.\n");
			ret = 1;
			goto out;
		}
		else if (ch != '\n') {
			err_unexpected_char(fp, ch, line , item);
			fprintf(stderr, "The checksum character length is invalid; Too many characters or missing newline.\n");
			ret = 1;
			goto out;
		}
		item++;
#endif /* CHECKSUM_RESULTS */

		line++;
		prev = *current;
		prevcount = progress->common_count;
	}
out:
	fclose(fp);
	if (ret)
		fprintf(stderr, "\n");
	return ret;
}

void save_checkpoint(vamp_t current, struct taskboard *progress)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "a");
	fprintf(fp, "%llu %llu", current, progress->common_count);

#ifdef CHECKSUM_RESULTS
	fprintf(fp, " ");
	for (int i = 0; i < progress->checksum->md_size; i++)
		fprintf(fp, "%02x", progress->checksum->md_value[i]);
#endif /* CHECKSUM_RESULTS */

	fprintf(fp, "\n");
	fclose(fp);
}

#endif /* USE_CHECKPOINT */
