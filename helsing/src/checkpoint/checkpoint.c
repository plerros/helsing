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

static int ftov(FILE *fp, vamp_t *number) // file to vamp_t
{
	assert(fp != NULL);
	assert(number != NULL);
	vamp_t ret = 0;
	bool flag = true;
	for (char ch = fgetc(fp); isdigit(ch) && !feof(fp); ch = fgetc(fp)) {
		flag = false;
		digit_t digit = ch - '0';
		if (willoverflow(ret, digit))
			return 1;
		ret = 10 * ret + digit;
	}
	*number = ret;
	return flag;
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
static int ftomd(FILE *fp, struct hash *ptr)
{
	assert(fp != NULL);
	assert(ptr->md_value != NULL);
	int err;
	uint8_t byte[2] = {0};
	for (int i = 0; i < ptr->md_size; i++) {
		err = hextobyte(fgetc(fp), &(byte[0]));
		if (err || feof(fp))
			return 1;
		err = hextobyte(fgetc(fp), &(byte[1]));
		if (err || feof(fp))
			return 1;
		ptr->md_value[i] = (byte[0] << 4) | byte[1];
	}
	return (fgetc(fp) != '\n' && !feof(fp));
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
	fprintf(fp, "%llu %llu", min, max);
	fclose(fp);
	return 0;
}

static void err_badline(vamp_t line)
{
	fprintf(stderr, "\n[ERROR] %s line %llu has bad data:\n", CHECKPOINT_FILE, line);
}

static void err_baditem(vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %llu item #%llu has bad data:\n", CHECKPOINT_FILE, line, item);
}

int load_checkpoint(vamp_t *min, vamp_t *max, vamp_t *current, struct taskboard *progress)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "r");
	assert(fp != NULL);

	vamp_t line = 1;

	if (ftov(fp, min) || ftov(fp, max)) {
		fclose(fp);
		err_badline(line);
		fprintf(stderr, "Input out of range: [0, %llu]\n\n", vamp_max);
	 	return 1;
	}

	if (*max < *min) {
		fclose(fp);
		err_badline(line);
		fprintf(stderr, "max < min\n\n");
		return 1;
	}

	*current = *min;
	progress->common_count = 0;
	line++;

	vamp_t prev = *current;
	vamp_t prevcount = 0;
	for (; !feof(fp); ) {
		if (ftov(fp, current)) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "number out of range: [0, %llu]\n\n", vamp_max);
			return 1;
		}
		if (*current < *min) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "%llu < %llu (below min)\n\n", *current,  *min);
			return 1;
		}
		if (*current > *max) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "%llu > %llu (above max)\n\n", *current,  *max);
			return 1;
		}
		if (*current <= prev && line != 2) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "%llu <= %llu (below previous)\n\n", *current, prev);
			return 1;
		}

		if (ftov(fp, &(progress->common_count))) {
			fclose(fp);
			err_baditem(line, 2);
			fprintf(stderr, "number out of range: [0, %llu]\n\n", vamp_max);
			return 1;
		}
		if (progress->common_count < prevcount && line != 2) {
			fclose(fp);
			err_baditem(line, 2);
			fprintf(stderr, "%llu < %llu (below previous)\n\n", progress->common_count, prevcount);
			return 1;
		}

#ifdef CHECKSUM_RESULTS
		if (ftomd(fp, progress->checksum)) {
			fclose(fp);
			err_baditem(line, 3);
			fprintf(stderr, "invalid checksum length\n\n");
			return 1;
		}
#endif /* CHECKSUM_RESULTS */

		line++;
		prev = *current;
		prevcount = progress->common_count;
	}
	fclose(fp);
	return 0;
}

void save_checkpoint(vamp_t current, struct taskboard *progress)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "a");
	fprintf(fp, "\n%llu %llu", current, progress->common_count);

#ifdef CHECKSUM_RESULTS
	fprintf(fp, " ");
	for (int i = 0; i < progress->checksum->md_size; i++)
		fprintf(fp, "%02x", progress->checksum->md_value[i]);
#endif /* CHECKSUM_RESULTS */

	fclose(fp);
}

#endif /* USE_CHECKPOINT */