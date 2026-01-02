// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#if ALG_CACHE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "helper.h"
#include "cache.h"

#define BITS_PER_NUMERAL(bits) ((double)(bits))/(double)(BASE - 1)
#define DIGBASE_UNSAFE(bits) (pow(2.0, BITS_PER_NUMERAL(bits)))
#define DIGBASE(bits) ((DIGBASE_UNSAFE(bits) >= ((double) DIGITS_T_MAX)) ? DIGITS_T_MAX : ((digits_t)DIGBASE_UNSAFE(bits)))

digits_t set_dig(fang_t number)
{
	length_t tmp[BASE] = {0};
	for (; number > 0; number /= BASE)
		tmp[number % BASE] += 1;

	const digits_t digbase_active_bits = DIGBASE(sizeof(digits_t) * CHAR_BIT);

	digits_t ret = 0;
	for (digit_t i = 1; i < BASE; i++) {
		OPTIONAL_ASSERT(DIGITS_T_MAX / digbase_active_bits >= ret);
		ret *= digbase_active_bits;
		OPTIONAL_ASSERT(DIGITS_T_MAX - tmp[i] >= ret);
		ret += tmp[i];
	}

	return ret;
}

static inline fang_t square(fang_t value)
{
	if (value > FANG_MAX / value)
		return FANG_MAX;
	
	return (value * value);
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
				.multiplicand_length = multiplicand_length,
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

		data.local.parts    = MULTIPLICAND_PARTITIONS;
		data.local.length   = data.global.multiplicand_length;
		data.local.iterator = data.global.multiplicand_iterator;
		for (int i = 0; i < data.local.parts; i++) {
			data.constant.idx_n = (i == data.local.parts-1);
			data.variable.index = i;
			length_t tmp = partition_exact(data, PARTITION_METHOD);
			if (tmp > cs)
				cs = tmp;
		}

		data.local.parts    = PRODUCT_PARTITIONS;
		data.local.length   = data.global.product_length;
		data.local.iterator = data.global.product_iterator;
		for (int i = 0; i < data.local.parts; i++) {
			data.constant.idx_n = (i == data.local.parts-1);
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

	/*
	 * Instead of calling set_dig(d) for all d, use dynamic programming to
	 * synthesize later results based on earlier ones, reducing complexity.
	 */

	fang_t window_min = 0;
	fang_t window_max = BASE;
	fang_t quotient  = 1;
	fang_t remainder = 1;

	fang_t j = 0;
	for (; j < new->size && j <= window_max; j++)
		new->dig[j] = set_dig(j);

	digits_t dig_quotient = new->dig[quotient];
	for (; j < new->size; j++) {
		if (j > window_max) {
			// This order of operations is faster
			window_min = window_max;
			quotient  = j / window_min;
			remainder = j % window_min;
			window_max = square(window_max);

			OPTIONAL_ASSERT(dig_quotient == new->dig[quotient]);
		}
		else if (remainder == window_min) {
			remainder = 0;
			quotient++;

			dig_quotient = new->dig[quotient];
		}
		new->dig[j] = new->dig[remainder] + dig_quotient;
		remainder++;
	}
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
 * part_scsg_rl:
 * (semi-constant, semi-global)
 *
 * partition x into n parts so that:
 * x <= (n-1) * A + B
 * if data_constant.idx_n is:
 * 	0, return A
 * 	1, return B
 */

static inline length_t part_scsg_rl_internal(
	struct partdata_constant_t data_constant,
	struct partdata_global_t data_global,
	length_t multiplicand_parts,
	length_t product_parts)
{
	length_t ret_multiplicand = data_global.multiplicand_length / multiplicand_parts;
	length_t ret_product = data_global.product_length / product_parts;

	if (data_constant.idx_n == true) {
		ret_multiplicand = data_global.multiplicand_length - ((multiplicand_parts - 1) * ret_multiplicand);
		ret_product = data_global.product_length - ((product_parts - 1) * ret_product);
	}
	length_t ret = ret_multiplicand;
	if (ret < ret_product)
		ret = ret_product;

	return ret;
}

static inline length_t part_vl_lr_internal(
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

static inline length_t part_vl_rl_internal(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	struct partdata_local_t tmp = data_local;
	if (tmp.length > tmp.parts)
		tmp.length = tmp.parts;
	data_local.length -= tmp.length;
	length_t ret = part_vl_lr_internal(data_variable, tmp);
	data_variable.index = data_local.parts - data_variable.index -1;
	ret +=part_vl_lr_internal(data_variable, data_local);
	return ret;
}

static inline length_t part_vl_l1r_internal(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	if (data_variable.reserve > data_local.length)
		data_variable.reserve = data_local.length;

	length_t ret = 0;
	data_local.length -= data_variable.reserve;
	ret += part_vl_lr_internal(data_variable, data_local);
	data_local.length = data_variable.reserve;
	ret += part_vl_lr_internal(data_variable, data_local);
	return(ret);
}

static inline length_t part_vl_r1l_internal(
	struct partdata_variable_t data_variable,
	struct partdata_local_t data_local)
{
	struct partdata_local_t tmp = data_local;
	if (tmp.length > tmp.parts)
		tmp.length = tmp.parts;
	data_local.length -= tmp.length;
	length_t ret = part_vl_lr_internal(data_variable, tmp);
	data_variable.index = data_local.parts - data_variable.index -1;
	ret +=part_vl_l1r_internal(data_variable, data_local);
	return ret;
}

#define PART_CG_BLUEPRINT(function_name, function_name_internal)                              \
length_t function_name(                                                                       \
	struct partdata_constant_t data_constant,                                             \
	struct partdata_global_t data_global)                                                 \
{                                                                                             \
	struct partdata_constant_t nth = data_constant;                                       \
	nth.idx_n = true;                                                                     \
	length_t part_n = function_name_internal(nth, data_global,                            \
				MULTIPLICAND_PARTITIONS, PRODUCT_PARTITIONS);                 \
                                                                                              \
	length_t multiplicand_limit = data_global.multiplicand_length;                        \
	multiplicand_limit -= data_global.multiplicand_iterator;                              \
	length_t product_limit = data_global.product_length;                                  \
	product_limit -= data_global.product_iterator;                                        \
	if ((MULTIPLICAND_PARTITIONS > 1) && (part_n > multiplicand_limit))                   \
		part_n = multiplicand_limit;                                                  \
	if ((PRODUCT_PARTITIONS > 1) && (part_n > product_limit))                             \
		part_n = product_limit;                                                       \
                                                                                              \
	if (data_constant.idx_n == true)                                                      \
		return part_n;                                                                \
                                                                                              \
	length_t multiplicand_parts = MULTIPLICAND_PARTITIONS;                                \
	if (MULTIPLICAND_PARTITIONS > 1) {                                                    \
		data_global.multiplicand_length -= part_n;                                    \
		multiplicand_parts -= 1;                                                      \
	}                                                                                     \
	length_t product_parts = PRODUCT_PARTITIONS;                                          \
	if (PRODUCT_PARTITIONS > 1) {                                                         \
		data_global.product_length -= part_n;                                         \
		product_parts -= 1;                                                           \
	}                                                                                     \
                                                                                              \
	data_constant.idx_n = false;                                                          \
	length_t ret = function_name_internal(data_constant, data_global,                     \
				multiplicand_parts, product_parts);                           \
	data_constant.idx_n = true;                                                           \
	length_t ret2 = function_name_internal(data_constant, data_global,                    \
				multiplicand_parts, product_parts);                           \
	if (ret2 > ret)                                                                       \
		ret = ret2;                                                                   \
	return ret;                                                                           \
}

#define PART_VL_BLUEPRINT(function_name, function_name_internal)   \
length_t function_name(                                            \
	struct partdata_variable_t data_variable,                  \
	struct partdata_local_t data_local)                        \
{                                                                  \
	struct partdata_variable_t nth = data_variable;            \
	nth.index = data_local.parts - 1;                          \
	length_t part_n = function_name_internal(nth, data_local); \
                                                                   \
	length_t local_limit = data_local.length;                  \
	local_limit -= data_local.iterator;                        \
	if ((data_local.parts > 1) && (part_n > local_limit))      \
		part_n = local_limit;                              \
                                                                   \
	if (data_variable.index == nth.index)                      \
		return part_n;                                     \
                                                                   \
	data_local.length -= part_n;                               \
	data_local.parts -= 1;                                     \
	return function_name_internal(data_variable, data_local);  \
}

PART_CG_BLUEPRINT(part_scsg_rl, part_scsg_rl_internal)
PART_VL_BLUEPRINT(part_vl_lr, part_vl_lr_internal)
PART_VL_BLUEPRINT(part_vl_rl, part_vl_rl_internal)
PART_VL_BLUEPRINT(part_vl_l1r, part_vl_l1r_internal)
PART_VL_BLUEPRINT(part_vl_r1l, part_vl_r1l_internal)

length_t partition_loose(struct partdata_all_t data, int method)
{
	switch(method) {
		case 0:
			return part_scsg_rl(data.constant, data.global);
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
