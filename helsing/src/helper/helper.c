// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdbool.h>
#include <limits.h>
#include <assert.h>

#include "configuration.h"
#include "configuration_adv.h"

/*
 * willoverflow:
 * Checks if (10 * x + digit) will overflow, without causing and overflow.
 */
bool willoverflow(vamp_t x, digit_t digit)
{
	assert(digit < 10);
	if (x > vamp_max / 10)
		return true;
	if (x == vamp_max / 10 && digit > vamp_max % 10)
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
#if SANITY_CHECK
	assert(exponent <= length(vamp_max) - 1);
#endif
	vamp_t power = 1;
	for (; exponent > 0; exponent--)
		power *= BASE;
	return power;
}

vamp_t div_roof (vamp_t x, vamp_t y)
{
	return (x/y + !!(x%y));
}