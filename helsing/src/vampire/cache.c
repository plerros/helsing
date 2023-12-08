// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021-2023 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#if ALG_CACHE
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "helper.h"
#include "cache.h"
#endif

#if ALG_CACHE

#define BITS_PER_NUMERAL(bits) ((double)(bits))/(double)(BASE - 1)
#define DIGBASE(bits) ((vamp_t) pow(2.0, BITS_PER_NUMERAL(bits)))

digits_t set_dig(fang_t number)
{
	digits_t ret = 0;
	digits_t tmp[BASE] = {0};
	for (; number > 0; number /= BASE)
		tmp[number % BASE] += 1;

	for (digit_t i = 1; i < BASE; i++) {
		OPTIONAL_ASSERT(tmp[i] < DIGBASE(ACTIVE_BITS));
		ret = ret * DIGBASE(ACTIVE_BITS) + tmp[i];
	}

	return ret;
}

void cache_new(struct cache **ptr, vamp_t min, vamp_t max)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct cache *new = malloc(sizeof(struct cache));
	if (new == NULL)
		abort();

	length_t cs = 0;
	length_t i = length(min);

	struct partdata_all_t data;
	struct partdata_constant_t data_constant = {
		.idx_n = false
	};
	struct partdata_global_t data_global = {
		.multiplicand_parts = 2,
		.product_parts = 3
	};

	do {
		data_global.multiplicand_length = div_roof(i, 2);
		data_global.product_length = i;

		data_global.multiplicand_iterator = length(BASE - 1);
		data_global.product_iterator = data_global.multiplicand_length + length(BASE - 1);

		data_constant.idx_n = false;
		length_t part_A = part_scsg_3(data_constant, data_global);
		data_constant.idx_n = true;
		length_t part_B = part_scsg_3(data_constant, data_global);
		if (part_A > cs)
			cs = part_A;
		if (part_B > cs)
			cs = part_B;
		i++;
	} while (i <= length(max));
	new->size = pow_v(cs);

	new->dig = malloc(sizeof(digits_t) * new->size);
	if (new->dig == NULL)
		abort();

	for (fang_t d = 0; d < new->size; d++)
		new->dig[d] = set_dig(d);
	*ptr = new;
}

void cache_free(struct cache *ptr)
{
	if (ptr == NULL)
		return;

	free(ptr->dig);
	free(ptr);
}

/*
 * Checks if the number can cause overflow.
 */

bool cache_ovf_chk(vamp_t max)
{
	double numeral_max;

#if (BASE != 2) // avoid division by 0
	numeral_max = log(max) / log(BASE);
#else
	numeral_max = log2(max);
#endif

	numeral_max = 2.0 * ceil(numeral_max / 2.0);
	return (numeral_max >= DIGBASE(ELEMENT_BITS) - 1);
}

/*
 * part_scsg_3:
 * (semi-constant, semi-global)
 *
 * partition x into 3 integers so that:
 * x <= 2 * A + B
 * if data_const.idx_n is:
 * 	0, return A
 * 	1, return B
 */

length_t part_scsg_3(
	struct partdata_constant_t data_constant,
	struct partdata_global_t data_global)
{
	length_t x = data_global.product_length;

	length_t B = x;
	length_t multiplicand_maxB = 0;
	if (data_global.multiplicand_length > data_global.multiplicand_iterator)
		multiplicand_maxB = data_global.multiplicand_length - data_global.multiplicand_iterator;
	length_t product_maxB = 0;
	if (data_global.product_length > data_global.product_iterator)
		product_maxB = data_global.product_length - data_global.product_iterator;

	if (B > multiplicand_maxB)
		B = multiplicand_maxB;

	if (B > product_maxB)
		B = product_maxB;

	length_t max = data_global.multiplicand_parts;
	if (max < data_global.product_parts)
		max = data_global.product_parts;

	length_t remainder = (x - B) % (max - 1);
	if (remainder > 0) {
		length_t adjust = (max - 1) - remainder;
		if (B >= adjust)
			B -= adjust;
		else
			x += adjust;
	}
	if (data_constant.idx_n)
		return B;

	length_t A = (x - B) / 2;
	return A;
}

#endif /* ALG_CACHE */
