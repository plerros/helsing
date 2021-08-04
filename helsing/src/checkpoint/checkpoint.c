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
// we assume ASCII / ASCII compatible

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
static int hextobyte(char ch, uint8_t *hex)
{
	if (!isxdigit(ch))
		return 1;

	switch (ch) {
		case '0':
			*hex = 0;
			break;
		case '1':
			*hex = 1;
			break;
		case '2':
			*hex = 2;
			break;
		case '3':
			*hex = 3;
			break;
		case '4':
			*hex = 4;
			break;
		case '5':
			*hex = 5;
			break;
		case '6':
			*hex = 6;
			break;
		case '7':
			*hex = 7;
			break;
		case '8':
			*hex = 8;
			break;
		case '9':
			*hex = 9;
			break;
		case 'a':
			*hex = 10;
			break;
		case 'b':
			*hex = 11;
			break;
		case 'c':
			*hex = 12;
			break;
		case 'd':
			*hex = 13;
			break;
		case 'e':
			*hex = 14;
			break;
		case 'f':
			*hex = 15;
			break;
	}
	return 0;
}

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
		uint8_t byte[2] = {0};
		for (int j = 0; j < 2; j++) {
			*ch = fgetc(fp);
			
			if (hextobyte(*ch, &(byte[j])))
				return -1;
		}
		ptr->md_value[i] = (byte[0] << 4) | byte[1];
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

static void err_unexpected_char(int ch)
{
	fprintf(stderr, "Unexpected ");
	switch (ch) {
		case ' ':
			fprintf(stderr, "space character");
			break;
		case '\t':
			fprintf(stderr, "tab character");
			break;
		case '\n':
			fprintf(stderr, "newline character");
			break;
		case EOF:
			fprintf(stderr, "end of file or missing newline.");
			break;
		default:
			fprintf(stderr, "character: ");
			if (isgraph(ch))
				fprintf(stderr, "%c", ch);
			else
				fprintf(stderr, "'%c'", ch);
			break;
	}
	fprintf(stderr, "\n\n");
}

static void err_switch(int err, vamp_t line, vamp_t item, int ch)
{
	err_baditem(line, item);
	switch (err) {
		case -1:
			err_unexpected_char(ch);
			break;
		case -2:
			fprintf(stderr, "Out of range: [0, %llu]\n\n", vamp_max);
			break;
	}
}

int load_checkpoint(vamp_t *min, vamp_t *max, vamp_t *current, struct taskboard *progress)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "r");
	assert(fp != NULL);

	vamp_t line = 1;
	vamp_t item = 1;
	int ch = 0;
	int ret = 0;
	int err = 0;

	err = ftov(fp, min, &ch);
	if (err) {
		err_switch(err, line, item, ch);
		ret = 1;
		goto load_checkpoint_exit;
	}
	else if (ch != ' ') {
		err_baditem(line, item);
		err_unexpected_char(ch);
		ret = 1;
		goto load_checkpoint_exit;
	}
	item++;

	err = ftov(fp, max, &ch);
	if (err) {
		err_switch(err, line, item, ch);
		ret = 1;
		goto load_checkpoint_exit;
	}
	else if (ch != '\n') {
		err_baditem(line, item);
		err_unexpected_char(ch);
		ret = 1;
		goto load_checkpoint_exit;
	}

	if (*max < *min) {
		fclose(fp);
		err_conflict(line, item);
		fprintf(stderr, "max < min\n\n");
		return 1;
	}

	*current = *min;
	progress->common_count = 0;
	line++;

	vamp_t prev = *current;
	vamp_t prevcount = 0;
	for (; !feof(fp); ) {
		item = 1;

		err = ftov(fp, current, &ch);
		if (err == -1 && ch == EOF && feof(fp)) {
			goto load_checkpoint_exit;
		}
		else if (err) {
			err_switch(err, line, item, ch);
			ret = 1;
			goto load_checkpoint_exit;
		}
		else if (ch != ' ') {
			err_baditem(line, item);
			err_unexpected_char(ch);
			ret = 1;
			goto load_checkpoint_exit;
		}

		if (*current < *min) {
			err_conflict(line, item);
			fprintf(stderr, "%llu < %llu (below min)\n\n", *current, *min);
			ret = 1;
			goto load_checkpoint_exit;
		}
		if (*current > *max) {
			err_conflict(line, item);
			fprintf(stderr, "%llu > %llu (above max)\n\n", *current, *max);
			ret = 1;
			goto load_checkpoint_exit;
		}
		if (*current <= prev && line != 2) {
			err_conflict(line, item);
			fprintf(stderr, "%llu <= %llu (below previous)\n\n", *current, prev);
			ret = 1;
			goto load_checkpoint_exit;
		}
		item++;

		err = ftov(fp, &(progress->common_count), &ch);
		if (err) {
			err_switch(err, line, item, ch);
			ret = 1;
			goto load_checkpoint_exit;
		}
#ifdef CHECKSUM_RESULTS
		else if (ch != ' ')
#else /* CHECKSUM_RESULTS */
		else if (ch != '\n')
#endif /* CHECKSUM_RESULTS */
		{
			err_baditem(line, item);
			err_unexpected_char(ch);
			ret = 1;
			goto load_checkpoint_exit;
		}
		if (progress->common_count < prevcount && line != 2) {
			err_conflict(line, item);
			fprintf(stderr, "%llu < %llu (below previous)\n\n", progress->common_count, prevcount);
			ret = 1;
			goto load_checkpoint_exit;
		}

#ifdef PROCESS_RESULTS
		if (progress->common_count > 0 && progress->common_count -1 > *current - *min) {
			err_conflict(line, item);
			fprintf(stderr, "More vampire numbers than numbers.\n\n");
			ret = 1;
			goto load_checkpoint_exit;
		}
#endif /* PROCESS_RESULTS */

		item++;

#ifdef CHECKSUM_RESULTS
		err = ftomd(fp, progress->checksum, &ch);
		if (err) {
			err_baditem(line, item);
			switch (err) {
				case -1:
					err_baditem(line, item);
					err_unexpected_char(ch);
					breal;
			}
			ret = 1;
			goto load_checkpoint_exit;
		}
		else if (ch != '\n') {
			err_baditem(line, item);
			fprintf(stderr, "Invalid checksum character length or missing newline.\n");
			ret = 1;
			goto load_checkpoint_exit;
		}
		item++;
#endif /* CHECKSUM_RESULTS */

		line++;
		prev = *current;
		prevcount = progress->common_count;
	}
load_checkpoint_exit:

	fclose(fp);
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