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
 * -1: EOF on start / empty.
 * -2: Overflow
 */

static int ftov(FILE *fp, vamp_t *ptr, char *ch) // file to vamp_t
{
	assert(fp != NULL);
	assert(ptr != NULL);

	*ch = fgetc(fp);

	if (feof(fp))
		return -1;

	vamp_t number = 0;
	for (; isdigit(*ch) && !feof(fp); *ch = fgetc(fp)) {
		digit_t digit = *ch - '0';
		if (willoverflow(number, digit))
			return -2;
		number = 10 * number + digit;
	}
	*ptr = number;
	return 0;
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
 * -1: EOF on start / empty.
 * -2: Non hex character
 */

static int ftomd(FILE *fp, struct hash *ptr, char *ch)
{
	assert(fp != NULL);
	assert(ptr->md_value != NULL);

	*ch = fgetc(fp);

	if (feof(fp))
		return -1;

	int err;
	uint8_t byte[2] = {0};
	for (int i = 0; i < ptr->md_size; i++) {
		for (int j = 0; j < 2; j++) {
			err = hextobyte(*ch, &(byte[j]));
			if (feof(fp))
				return -1;
			if (err)
				return -2;
			*ch = fgetc(fp);
		}
		ptr->md_value[i] = (byte[0] << 4) | byte[1];
	}
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

int load_checkpoint(vamp_t *min, vamp_t *max, vamp_t *current, struct taskboard *progress)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "r");
	assert(fp != NULL);

	vamp_t line = 1;
	vamp_t item = 1;
	char ch = 0;

	switch (ftov(fp, min, &ch)) {
		case 0:
			if (ch == ' ')
				break;
			
			err_baditem(line, item);
			if (feof(fp))
				fprintf(stderr, "Unexpected end of file.\n\n");
			else
				fprintf(stderr, "Non numeric character: '%c'\n\n", ch);

			fclose(fp);
			return 1;
		case -1:
			fclose(fp);
			err_baditem(line, item);
			fprintf(stderr, "Unexpected end of file.\n\n");
			return 1;
		case -2:
			fclose(fp);
			err_baditem(line, item);
			fprintf(stderr, "Out of range: [0, %llu]\n\n", vamp_max);
			return 1;
	}

	switch (ftov(fp, max, &ch)) {
		case 0:
			if (ch == '\n')
				break;
			
			err_baditem(line, item);
			if (feof(fp))
				fprintf(stderr, "Unexpected end of file.\n\n");
			else
				fprintf(stderr, "Non numeric character: '%c'\n\n", ch);

			fclose(fp);
			return 1;
		case -1:
			fclose(fp);
			err_baditem(line, item);
			fprintf(stderr, "Unexpected end of file.\n\n");
			return 1;
		case -2:
			fclose(fp);
			err_baditem(line, item);
			fprintf(stderr, "Out of range: [0, %llu]\n\n", vamp_max);
			return 1;
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

		switch (ftov(fp, current, &ch)) {
			case 0:
				if (ch == ' ')
					break;
				
				err_baditem(line, item);
				if (feof(fp))
					fprintf(stderr, "Unexpected end of file.\n\n");
				else
					fprintf(stderr, "Non numeric character: '%c'\n\n", ch);

				fclose(fp);
				return 1;
			case -1:
				goto load_checkpoint_exit;
			case -2:
				fclose(fp);
				err_baditem(line, item);
				fprintf(stderr, "Out of range: [0, %llu]\n\n", vamp_max);
				return 1;
		}

		if (*current < *min) {
			fclose(fp);
			err_conflict(line, item);
			fprintf(stderr, "%llu < %llu (below min)\n\n", *current, *min);
			return 1;
		}
		if (*current > *max) {
			fclose(fp);
			err_conflict(line, item);
			fprintf(stderr, "%llu > %llu (above max)\n\n", *current, *max);
			return 1;
		}
		if (*current <= prev && line != 2) {
			fclose(fp);
			err_conflict(line, item);
			fprintf(stderr, "%llu <= %llu (below previous)\n\n", *current, prev);
			return 1;
		}
		item++;

		switch (ftov(fp, &(progress->common_count), &ch)) {
			case 0:

#ifdef CHECKSUM_RESULTS
				if (ch == ' ')
					break;
				
				err_baditem(line, item);
				if (feof(fp))
					fprintf(stderr, "Unexpected end of file.\n\n");
				else
					fprintf(stderr, "Non numeric character: '%c'\n\n", ch);
#else /* CHECKSUM_RESULTS */
				if (ch == '\n')
					break;
				
				err_baditem(line, item);
				if (feof(fp) || ch == ' ')
					fprintf(stderr, "Missing newline.\n\n");
				else
					fprintf(stderr, "Non numeric character: '%c'\n\n", ch);
#endif /* CHECKSUM_RESULTS */

				fclose(fp);
				return 1;
			case -1:
				fclose(fp);
				err_baditem(line, item);
				fprintf(stderr, "Unexpected end of file.\n\n");
				return 1;
			case -2:
				fclose(fp);
				err_baditem(line, item);
				fprintf(stderr, "Out of range: [0, %llu]\n\n", vamp_max);
				return 1;
		}
		if (progress->common_count < prevcount && line != 2) {
			fclose(fp);
			err_conflict(line, item);
			fprintf(stderr, "%llu < %llu (below previous)\n\n", progress->common_count, prevcount);
			return 1;
		}

#ifdef PROCESS_RESULTS
		if (progress->common_count > 0 && progress->common_count -1 > *current - *min) {
			fclose(fp);
			err_conflict(line, item);
			fprintf(stderr, "More vampire numbers than numbers.\n\n");
			return 1;
		}
#endif /* PROCESS_RESULTS */

		item++;

#ifdef CHECKSUM_RESULTS
		switch (ftomd(fp, progress->checksum, &ch)) {
			case 0:
				if (ch == '\n')
					break;
				err_baditem(line, item);
				if (feof(fp) || ch == ' ')
					fprintf(stderr, "Missing newline.\n\n");
				else
					fprintf(stderr, "Invalid checksum character length.\n\n");
				return 1;
				
			case -1:
				fclose(fp);
				err_baditem(line, item);
				fprintf(stderr, "Unexpected end of file.\n\n");
				return 1;
			case -2:
				fclose(fp);
				err_baditem(line, item);
				if (ch == '\n')
					fprintf(stderr, "Invalid checksum character length.\n\n");
				else
					fprintf(stderr, "Non hex character: %c\n\n", ch);
				return 1;
		}
		item++;
#endif /* CHECKSUM_RESULTS */

		line++;
		prev = *current;
		prevcount = progress->common_count;
	}
load_checkpoint_exit:

	fclose(fp);
	return 0;
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