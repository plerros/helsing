// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>	// isdigit
#include <openssl/evp.h>

#include "configuration.h"
#include "helper.h"

#if defined(USE_CHECKPOINT) && USE_CHECKPOINT

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

static uint8_t chartohex(char in)
{
	if (isdigit(in))
		return (in - '0');
	else
		return (in - 'a' + 10);
	return 0;
}
static int ftostr(FILE *fp, unsigned char *str)
{
	assert(fp != NULL);
	assert(str != NULL);
	int i = 0;
	bool flag = true;
	for (char ch = fgetc(fp); isxdigit(ch) && !feof(fp) && i < EVP_MAX_MD_SIZE; ch = fgetc(fp)) {
		flag = false;
		char ch2 = fgetc(fp);
		if (isxdigit(ch2) && !feof(fp)) {
			str[i] = (chartohex(ch) << 4) | chartohex(ch2);
		} else {
			return 1;
		}
		//fprintf(stderr, "read[%c][%c]", ch, ch2);
		//fprintf(stderr, " write [%02x]\n", str[i]);
		i++;
	}
	return flag;

}

void touch_checkpoint(vamp_t min, vamp_t max)
{
	FILE *fp;
	fp = fopen(CHECKPOINT_FILE, "r");
	if (fp != NULL) {
		fclose(fp);
		fprintf(stderr, "%s already exists\n", CHECKPOINT_FILE);
		exit(0);
	}
	fp = fopen(CHECKPOINT_FILE, "w+");
	fprintf(fp, "%llu %llu", min, max);
	fclose(fp);
}

static void err_badline(vamp_t line)
{
	fprintf(stderr, "\n[ERROR] %s line %llu has bad data:\n", CHECKPOINT_FILE, line);
}

static void err_baditem(vamp_t line, vamp_t item)
{
	fprintf(stderr, "\n[ERROR] %s line %llu item #%llu has bad data:\n", CHECKPOINT_FILE, line, item);
}

void load_checkpoint(vamp_t *min, vamp_t *max, vamp_t *current, vamp_t *count, unsigned char *md_value)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "r");
	assert(fp != NULL);

	vamp_t line = 1;

	if (ftov(fp, min) || ftov(fp, max)) {
		fclose(fp);
		err_badline(line);
		fprintf(stderr, "Input out of range: [0, %llu]\n\n", vamp_max);
	 	exit(0);
	}

	if (*max < *min) {
		fclose(fp);
		err_badline(line);
		fprintf(stderr, "max < min\n\n");
		exit(0);
	}

	*current = *min;
	*count = 0;
	line++;

	vamp_t prev = *current;
	vamp_t prevcount = 0;
	for (; !feof(fp); ) {
		if (ftov(fp, current)) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "number out of range: [0, %llu]\n\n", vamp_max);
			exit(0);
		}
		if (*current < *min) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "%llu < %llu (below min)\n\n", *current,  *min);
			exit(0);
		}
		if (*current > *max) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "%llu > %llu (above max)\n\n", *current,  *max);
			exit(0);
		}
		if (*current <= prev && line != 2) {
			fclose(fp);
			err_baditem(line, 1);
			fprintf(stderr, "%llu <= %llu (below previous)\n\n", *current, prev);
			exit(0);
		}

		if (ftov(fp, count)) {
			fclose(fp);
			err_baditem(line, 2);
			fprintf(stderr, "number out of range: [0, %llu]\n\n", vamp_max);
			exit(0);
		}
			//fprintf(stderr, "%u\n", EVP_MAX_MD_SIZE););
		if (*count < prevcount && line != 2) {
			fclose(fp);
			err_baditem(line, 2);
			fprintf(stderr, "%llu < %llu (below previous)\n\n", *count, prevcount);
			exit(0);
		}

		if (ftostr(fp, md_value)) {
			fclose(fp);
			err_baditem(line, 3);
			fprintf(stderr, "number out of range: [0, %llu]\n\n", vamp_max);
			exit(0);
		}
		//for (unsigned int i = 0; i < EVP_MAX_MD_SIZE; i++)
		//	fprintf(stderr, "%02x", md_value[i]);
		//fprintf(stderr, "\n");

		line++;
		prev = *current;
		prevcount = *count;
	}
	fclose(fp);
}

void save_checkpoint(vamp_t current, vamp_t count, unsigned char *md_value)
{
	FILE *fp = fopen(CHECKPOINT_FILE, "a");
	fprintf(fp, "\n%llu %llu ", current, count);

#ifdef CHECKSUM_RESULTS
	for (unsigned int i = 0; i < EVP_MAX_MD_SIZE; i++)
		fprintf(fp, "%02x", md_value[i]);
#endif

	fclose(fp);
}

#endif /* defined(USE_CHECKPOINT) && USE_CHECKPOINT */