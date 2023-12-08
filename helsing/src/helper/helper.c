// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdbool.h>
#include <limits.h>
#include <assert.h>

#include "configuration.h"
#include "configuration_adv.h"

void no_args() {};

/*
 * willoverflow:
 * Checks if (10 * x + digit) will overflow, without causing and overflow.
 * Should only be used for input checking, where the numeral base is 10.
 */
bool willoverflow(vamp_t x, vamp_t limit, digit_t digit)
{
	assert(digit < 10);
	if (x > limit / 10)
		return true;
	if (x == limit / 10 && digit > limit % 10)
		return true;
	return false;
}

length_t length(vamp_t x)
{
	length_t length = 1;
	for (; x >= BASE; x /= BASE)
		length++;
	return length;
}

vamp_t pow_v(length_t exponent) // pow for vamp_t.
{
	OPTIONAL_ASSERT(exponent <= length(VAMP_MAX) - 1);
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
