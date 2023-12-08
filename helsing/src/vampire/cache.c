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

	do {
		length_t multiplicand_length =  div_roof(i, 2);
		struct partdata_all_t data = {
			.constant = {
				.idx_n = false
			},
			.variable = {
				.index = 0,
				.reserve = 1
			},
			.global = {
				.multiplicand_parts = 2,
				.multiplicand_length = multiplicand_length,
				.product_parts = 3,
				.product_length = i,
				.multiplicand_iterator = length(BASE - 1),
				.product_iterator = multiplicand_length + length(BASE - 1)
			},
			.local = {
				.parts = 0,
				.length = 0,
				.iterator = 0
			}
		};

		data.local.parts    = data.global.multiplicand_parts;
		data.local.length   = data.global.multiplicand_length;
		data.local.iterator = data.global.multiplicand_iterator;
		for (int i = 0; i < 2; i++) {
			data.constant.idx_n = (i == 2-1);
			data.variable.index = i;
			length_t tmp = partition_exact(data, PARTITION_METHOD);
			if (tmp > cs)
				cs = tmp;
		}

		data.local.parts    = data.global.product_parts;
		data.local.length   = data.global.product_length;
		data.local.iterator = data.global.product_iterator;
		for (int i = 0; i < 3; i++) {
			data.constant.idx_n = (i == 3-1);
			data.variable.index = i;
			length_t tmp = partition_exact(data, PARTITION_METHOD);
			if (tmp > cs)
				cs = tmp;
		}
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

length_t part_vl_lr(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	if (data_local.parts == 0)
		return 0;

	length_t ret = data_local.length / data_local.parts;
	if (data_variable.index < data_local.length % data_local.parts)
		ret += 1;

	return ret;
}

length_t part_vl_rl(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	data_variable.index = data_local.parts - data_variable.index -1;
	return(part_vl_lr(data_variable, data_local));
}

length_t part_vl_l1r(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	if (data_variable.reserve > data_local.length)
		data_variable.reserve = data_local.length;

	length_t ret = 0;
	data_local.length -= data_variable.reserve;
	ret += part_vl_lr(data_variable, data_local);
	data_local.length = data_variable.reserve;
	ret += part_vl_lr(data_variable, data_local);
	return(ret);
}

length_t part_vl_r1l(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	data_variable.index = data_local.parts - data_variable.index -1;
	return(part_vl_l1r(data_variable, data_local));
}

length_t partition_loose(struct partdata_all_t data, int method)
{
	switch(method) {
		case 0:
			return part_scsg_3(data.constant, data.global);
		case 1:
			return part_vl_lr(data.variable, data.local);
		case 2:
			return part_vl_rl(data.variable, data.local);
		case 3:
			return part_vl_l1r(data.variable, data.local);
		case 4:
			return part_vl_r1l(data.variable, data.local);
		default:
			abort();
	}
}

length_t partition_exact(struct partdata_all_t data, int method)
{
	length_t remainder = data.local.length;
	length_t max = data.variable.index;
	length_t ret = 0;
	for (length_t i = 0; i <= max; i++) {
		data.constant.idx_n = (i == data.local.parts - 1);
		data.variable.index = i;
		ret = partition_loose(data, method);

		if (i == max)
			break;

		if (ret > remainder)
			remainder = 0;
		else
			remainder -= ret;
	}
	if (remainder < ret)
		return remainder;

	return ret;
}

#endif /* ALG_CACHE */
