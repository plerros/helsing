// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2023 Pierro Zachareas
 */

#ifndef HELSING_CACHE_H
#define HELSING_CACHE_H

#include "configuration.h"
#include "configuration_adv.h"
#include <stdbool.h>

#if ALG_CACHE
struct cache
{
	digits_t *dig;
	fang_t size;
};
digits_t set_dig(fang_t number);
void cache_new(struct cache **ptr, vamp_t min, vamp_t max);
void cache_free(struct cache *ptr);
bool cache_ovf_chk(vamp_t max);
#else /* !ALG_CACHE */
struct cache
{
};
static inline digits_t set_dig(__attribute__((unused)) fang_t number)
{
	return 0;
}
static inline void cache_new(
	__attribute__((unused)) struct cache **ptr,
	__attribute__((unused)) vamp_t min,
	__attribute__((unused)) vamp_t max)
{
}
static inline void cache_free(__attribute__((unused)) struct cache *ptr)
{
}
static inline bool cache_ovf_chk(__attribute__((unused)) vamp_t max)
{
	return false;
}
#endif /* ALG_CACHE */


#if ALG_CACHE
/*
 * Partition
 *
 * We are tasked to partition numbers.
 * For example if we wanted 3 partitions of the number 12345678, we could do two
 * partitions of length 3 and one of length 2:
 * 12345678 -> 123, 456, 78
 * (8)         (3)  (3)  (2)
 *
 * We can partition the numbers for loose or exact fit
 * 	1) loose fit, length(number) <= sum(partitions)
 * 	2) exact fit, length(number) == sum(partitions)
 *
 * The rest of the code assumes loose, and works with either.
 *
 * In helsing, the numbers we partition are the multiplicand and the product.
 * Let's partition them as such:
 * 	mult[0], mult[1], ... mult[n]
 * 	prod[0], prod[1], ... prod[m]
 *
 * The partitioning methods can be constant, semi-constant & variable:
 * 	number:	n partitions
 * 	1) constant:
 * 		number[0] = number[1] = ... = number[n]
 *
 * 	2) semi-constant:
 * 		number[0] = number[1] = ... = number[n-1]
 *
 * 	3) variable
 *
 * The partitioning methods can be global, semi-global & local:
 * 	mult:	n partitions
 * 	prod:	m partitions
 * 	k = min(n, m)
 *
 * 	1) global:
 * 		mult[0] = prod[0]
 * 		mult[1] = prod[1]
 * 		...
 * 		mult[k] = prod[k]
 *
 * 	2) semi-global:
 * 		mult[0] = prod[0]
 * 		mult[1] = prod[1]
 * 		...
 * 		mult[k-1] = prod[k-1]
 *
 * 	3) local
 *
 * Assuming that m,n > 1, all constant, semi-constant, global and semi-global
 * methods are loose-fit.
 */

struct partdata_t
{
};

struct partdata_constant_t
{
	bool idx_n; // is index == n?
};

struct partdata_variable_t
{
	length_t index;
};

struct partdata_global_t
{
	length_t multiplicand_parts;
	length_t multiplicand_length;
	length_t multiplicand_iterator;
	length_t product_parts;
	length_t product_length;
	length_t product_iterator;
};

struct partdata_local_t
{
	length_t parts;
	length_t length;
	length_t iterator;
};

struct partdata_other_t
{
};

struct partdata_all_t
{
	struct partdata_constant_t	constant;
	struct partdata_variable_t	variable;
	struct partdata_global_t  	global;
	struct partdata_local_t   	local;
	struct partdata_other_t   	other;
};

// Semi-Constant & Semi-Global
__attribute__((const))
length_t part_scsg_3(
	struct partdata_constant_t data_const,
	struct partdata_global_t data_glob);
#endif /* ALG_CACHE */
#endif /* HELSING_CACHE_H */
