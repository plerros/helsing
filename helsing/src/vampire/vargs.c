// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021-2023 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"
#include "llnode.h"
#include "array.h"
#include "cache.h"
#include "vargs.h"

static bool notrailingzero(fang_t x)
{
	return ((x % BASE) != 0);
}

static fang_t sqrtv_floor(vamp_t x) // vamp_t sqrt to fang_t.
{
	vamp_t x2 = x / 2;
	vamp_t root = x2;
	if (root > 0) {
		vamp_t tmp = (root + x / root) / 2;
		while (tmp < root) {
			root = tmp;
			tmp = (root + x / root) / 2;
		}
		return root;
	}
	return x;
}

static fang_t sqrtv_roof(vamp_t x)
{
	if (x == 0)
		return 0;

	fang_t root = sqrtv_floor(x);
	if (root == FANG_MAX)
		return root;

	return (x / root);
}

/*
 * disqualify_mult:
 *
 * Disqualify ineligible values before congruence_check.
 * Currently suppoted numerical bases: 2~10.
 */

static bool disqualify_mult(vamp_t x)
{
	bool ret = false;
	switch (BASE) {
		case 2:
			ret = false;
			break;
		case 7:	{
			int tmp = x % (BASE - 1);
			ret = (tmp == 1 || tmp == 3 || tmp == 4 || tmp == 5);
			break;
		}
		case 10:
			ret = (x % 3 == 1);
			break;
		default:
			/*
			 * A represents the last bit of multiplier
			 * B represents the last bit of multiplicand
			 *
			 * A B  A+B  A*B  Match
			 * 0 0   0    0   true
			 * 0 1   1    0   false
			 * 1 0   1    0   false
			 * 1 1 (1)0   1   false
			 *
			 * If BASE-1 is a power of two, we can safely disqualify
			 * the cases where A is 1.
			 */
			if (((BASE - 1) & (BASE - 2)) == 0)
				ret = x % 2;
			else
				ret = (x % (BASE - 1) == 1);
	}
	return ret;
}

// Modulo base-1 lack of congruence
static bool congruence_check(vamp_t x, vamp_t y)
{
	return ((x + y) % (BASE - 1) != (x * y) % (BASE - 1));
}

void vargs_new(struct vargs **ptr, struct cache *digptr)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct vargs *new = malloc(sizeof(struct vargs));
	if (new == NULL)
		abort();

	new->digptr = digptr;
	new->local_count = 0;
	new->result = NULL;
	*ptr = new;
}

void vargs_free(struct vargs *args)
{
	if (args == NULL)
		return;

	array_free(args->result);
	free(args);
}

void vargs_reset(struct vargs *args)
{
	args->local_count = 0;
	array_free(args->result);
	args->result = NULL;
}

#if ALG_NORMAL // when false we use the empty functions in vargs.h

static void alg_normal_set(fang_t multiplier, length_t (*mult_array)[BASE])
{
	for (digit_t i = 0; i < BASE; i++)
		(*mult_array)[i] = 0;

	for (fang_t i = multiplier; i > 0; i /= BASE)
		(*mult_array)[i % BASE] += 1;
}

static void alg_normal_check(
	length_t mult_array[BASE],
	fang_t multiplicand,
	vamp_t product,
	int *result)
{
	uint16_t product_array[BASE] = {0};
	for (vamp_t p = product; p > 0; p /= BASE)
		product_array[p % BASE] += 1;

	for (digit_t i = 0; i < BASE; i++)
		if (product_array[i] < mult_array[i])
			goto out;

	digit_t temp;
	for (fang_t m = multiplicand; m > 0; m /= BASE) {
		temp = m % BASE;
		if (product_array[temp] == 0)
			goto out;
		else
			product_array[temp]--;
	}
	for (digit_t i = 0; i < (BASE - 1); i++)
		if (product_array[i] != mult_array[i])
			goto out;

	(*result) += 1;
out:
	return;
}

#endif /* ALG_NORMAL */

#if ALG_CACHE // when false we use the empty functions in vargs.h

/*
 * We could just allocate the entire dig[] array, and then do:
 *
 * 	for (; multiplicand <= multiplicand_max; multiplicand += BASE - 1) {
 * 		if (dig[multiplier] + dig[multiplicand] == dig[product]) {
 * 			...
 * 		}
 * 		product += product_iterator;
 *		multiplicand += BASE-1;
 *	}
 *
 * This would work just fine.
 * The only problem is that the array would be way too
 * big to fit in most l3 caches and we would waste a
 * majority of time loading data from memory.
 *
 * If we 'partition' the numbers (123 -> 12, 3), we can
 * make the array much smaller.
 *
 * Of course, 'partitioning' requires some computation,
 * but we are already waiting for memory load
 * operations, and we might as well put the wasted
 * cycles to good use.
 *
 * We 'partition' like this:
 * 	multiplier: multiplier[0]
 * 	multiplicand: multiplicand[0], multiplicand[1]
 *
 * 	product:          product[0],   product[1],   product[2]
 * 	product iterator: product_iterator[0], product_iterator[1], product_iterator[2]
 *
 * Because it seems to perform pretty well.
 */

struct num_part
{
	fang_t number;
	fang_t iterator;
};

struct alg_cache
{
	fang_t mod;
	digits_t *digits_array;
	digits_t dig_multiplier;	// doesn't change when we iterate
	// multiplicand iterator is BASE - 1
	struct num_part multiplicand[2];
	struct num_part product[3];
};

static void alg_cache_init(struct alg_cache *ptr, fang_t mod, struct cache *cache)
{
	OPTIONAL_ASSERT(ptr != NULL);
	ptr->mod = mod;

	ptr->digits_array = NULL;
	if (cache != NULL)
		ptr->digits_array = cache->dig;
}

static void alg_cache_split(
	vamp_t number,
	fang_t mod,
	vamp_t iterator,
	struct num_part *arr,
	size_t n)
{	
	if (n == 0)
		return;

	for (size_t i = 0; i < n - 1; i++) {
		arr[i].number = number % mod;
		number /= mod;
		arr[i].iterator = iterator % mod;
		iterator /= mod;
	}
	arr[n - 1].number = number; // number >= number % mod
	arr[n - 1].iterator = iterator;
}

static void alg_cache_set(
	struct alg_cache *ptr,
	fang_t multiplier,
	fang_t multiplicand,
	vamp_t product,
	vamp_t product_iterator)
{
	/*
	 * dig_multiplier = digits_array[multiplier];
	 * Each dig_multiplier is calculated and accessed only once, we don't need to store them in memory.
	 * We can calculate dig_multiplier on the spot and make the dig array 10 times smaller.
	 */

	ptr->dig_multiplier = set_dig(multiplier);
	alg_cache_split(multiplicand, ptr->mod, BASE-1, ptr->multiplicand, 2);
	alg_cache_split(product, ptr->mod, product_iterator, ptr->product, 3);

	/*
	 * We can improve the runtime even further by removing product_iterator[2].
	 * If product_iterator[2] is always 0, we don't need it.
	 *
	 * product_iterator[2] = (product_iterator / power_a) / power_a
	 *
	 * product_iterator[2] has 0 digits, product_iterator has n+1, and we are going to solve for power_a:
	 *
	 * 0 >= (n+1 - x) - x
	 * x >= n+1 - x
	 */

	if (ptr->multiplicand[0].iterator != BASE-1)
		abort();	// Let the compiler know that this is constant
	OPTIONAL_ASSERT(ptr->product[3 - 1].iterator == 0);
}

static void alg_cache_check(struct alg_cache *ptr, int *result)
{
	const digits_t *digits_array = ptr->digits_array;

	digits_t a = ptr->dig_multiplier;
	for (int i = 0; i < 2; i++)
		a += digits_array[ptr->multiplicand[i].number];

	digits_t b = digits_array[ptr->product[0].number];
	for (int i = 1; i < 3; i++)
		b += digits_array[ptr->product[i].number];

	if (a == b)
		(*result) += 1;
}

static inline void alg_cache_iterate(
	fang_t mod,
	struct num_part *arr,
	int elements)
{
	int i = 0;
	for (; i < elements - 1; i++) {
		arr[i].number += arr[i].iterator;
		if (arr[i].number >= mod) {
			arr[i].number -= mod;
			arr[i + 1].number += 1;
		}
	}
}

static void alg_cache_iterate_all(struct alg_cache *ptr)
{
	alg_cache_iterate(ptr->mod, ptr->multiplicand, 2);
	alg_cache_iterate(ptr->mod, ptr->product, 3);
}

#endif /* ALG_CACHE */


void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax)
{
	struct llnode *ll = NULL;
	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

	struct alg_cache ag_data;
	alg_cache_init(&ag_data, pow_v(partition3(length(max))), args->digptr);

	length_t mult_array[BASE];

	for (fang_t multiplier = fmax; multiplier >= min_sqrt && multiplier > 0; multiplier--) {
		if (disqualify_mult(multiplier))
			continue;

		fang_t multiplicand = div_roof(min, multiplier); // fmin * fmax <= min - BASE^n
		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier > max_sqrt)
			multiplicand_max = max / multiplier;
		else
			multiplicand_max = multiplier;
			// multiplicand <= multiplier: 5267275776 = 72576 * 72576.

		while (multiplicand <= multiplicand_max && congruence_check(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand > multiplicand_max)
			continue;
		/*
		 * If multiplier has n digits, then product_iterator has at most n+1 digits.
		 */
		vamp_t product_iterator = multiplier;
		product_iterator *= BASE - 1; // <= (BASE-1) * (2^32)
		vamp_t product = multiplier;
		product *= multiplicand; // avoid overflow

		alg_cache_set(&ag_data, multiplier, multiplicand, product, product_iterator);

		alg_normal_set(multiplier, &mult_array);

		for (; multiplicand <= multiplicand_max; multiplicand += BASE - 1) {
			int result = 0;

			alg_normal_check(mult_array, multiplicand, product, &result);
			alg_cache_check(&ag_data, &result);

			if (result && (mult_zero || notrailingzero(multiplicand))) {
				vargs_iterate_local_count(args);
				vargs_print_results(product, multiplier, multiplicand);
				llnode_add(&(ll), product);
			}
			alg_cache_iterate_all(&ag_data);
			product += product_iterator;
		}
	}
	array_new(&(args->result), ll, &(args->local_count));
	llnode_free(ll);
	return;
}
