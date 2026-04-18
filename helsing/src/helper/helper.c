// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "configuration.h"
#include "configuration_adv.h"

void no_args() {};

/*
 * willoverflow:
 * Checks if (10 * x + digit) will overflow, without causing and overflow.
 * Should only be used for input checking, where the numeral base is 10.
 */
bool willoverflow(bimax_t x, bimax_t limit, digit_t digit)
{
	assert(digit < 10);
	if (x > limit / 10)
		return true;
	if (x == limit / 10 && digit > limit % 10)
		return true;
	return false;
}

length_t length(bimax_t x)
{
	length_t length = 1;
	for (; x >= BASE; x /= BASE)
		length++;
	return length;
}

void printany(FILE *fp, bimax_t value)
{
	if (value > 9)
		printany(fp, value / 10);

	fprintf(fp, "%d", (int)(value % 10));
}

bimax_t pow_any(length_t exponent) // pow for vamp_t.
{
	OPTIONAL_ASSERT(exponent <= length(BIMAX_MAX()) - 1);
	bimax_t power = 1;
	for (; exponent > 0; exponent--)
		power *= BASE;
	return power;
}

vamp_t pow_v(length_t exponent) // pow for vamp_t.
{
	OPTIONAL_ASSERT(exponent <= length(VAMP_MAX()) - 1);
	vamp_t power = 1;
	for (; exponent > 0; exponent--)
		power *= BASE;
	return power;
}

vamp_t get_min(vamp_t min, vamp_t max)
{
	if (length(min) % 2) {
		length_t min_length = length(min);
		if (min_length < length(max))
			min = pow_v(min_length);
		else
			min = max;
	}
	return min;
}

vamp_t get_max(vamp_t min, vamp_t max)
{
	if (length(max) % 2) {
		length_t max_length = length(max);
		if (max_length > length(min))
			max = pow_v(max_length - 1) - 1;
		else
			max = min;
	}
	return max;
}

vamp_t div_roof(vamp_t x, vamp_t y)
{
	return (x/y + !!(x%y));
}

/*
 * helsing_fprint:
 * 	'a':	bimax_t
 * 	'f':	fang_t
 * 	's':	string
 * 	'v':	vamp_t
 * 	'z':	size_t
 */

void helsing_fprint(FILE *fp, char *formats, ...)
{
	va_list args;
	

	for (va_start(args, formats); *formats != '\0'; formats++) {
		switch (*formats) {
			case 'a':
				printany(fp, va_arg(args, bimax_t));
				break;
			case 'f':
				printany(fp, va_arg(args, fang_t));
				break;
			case 's':
				fprintf(fp, "%s", va_arg(args, char *));
				break;
			case 'v':
				printany(fp, va_arg(args, vamp_t));
				break;
			case 'z':
				fprintf(fp, "%zu", va_arg(args, size_t));
				break;
			default:
				assert(1);
		}
	}
	va_end(args);
}
