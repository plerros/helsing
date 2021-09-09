// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021 Pierro Zachareas
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

#if SANITY_CHECK
#include <assert.h>
#endif

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
	if (root == fang_max)
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
		case 7:
			{
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
#if SANITY_CHECK
	assert(ptr != NULL);
	assert (*ptr == NULL);
#endif

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

void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax)
{
	struct llnode *ll = NULL;
	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

#if CACHE
	length_t length_max = length(max);
	length_t length_a = length_max / 3;
	if (length_max < 9)
		length_a += length_max % 3;

	fang_t power_a = pow_v(length_a);
	digits_t *dig = args->digptr->dig;
#endif

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

		if (multiplicand <= multiplicand_max) {
			vamp_t product_iterator = multiplier;
			product_iterator *= BASE - 1; // <= (BASE-1) * (2^32)
			vamp_t product = multiplier;
			product *= multiplicand; // avoid overflow

#if CACHE

			fang_t step0 = product_iterator % power_a;
			fang_t step1 = product_iterator / power_a;

			/*
			 * digd = dig[multiplier];
			 * Each digd is calculated and accessed only once, we don't need to store them in memory.
			 * We can calculate digd on the spot and make the dig array 10 times smaller.
			 */

			digits_t digd = set_dig(multiplier);

			fang_t e0 = multiplicand % power_a;
			fang_t e1 = multiplicand / power_a;

			fang_t de0 = product % power_a;
			fang_t de1 = (product / power_a) % power_a;
			fang_t de2 = (product / power_a) / power_a;

			for (; multiplicand <= multiplicand_max; multiplicand += BASE - 1) {
				if (digd + dig[e0] + dig[e1] == dig[de0] + dig[de1] + dig[de2])
					if (mult_zero || notrailingzero(multiplicand)) {
						vargs_iterate_local_count(args);
						vargs_print_results(product, multiplier, multiplicand);
						llnode_add(&(ll), product);
					}
				product += product_iterator;
				e0 += BASE - 1;
				if (e0 >= power_a) {
					e0 -= power_a;
					e1 += 1;
				}
				de0 += step0;
				if (de0 >= power_a) {
					de0 -= power_a;
					de1 += 1;
				}
				de1 += step1;
				if (de1 >= power_a) {
					de1 -= power_a;
					de2 += 1;
				}
			}

#else /* CACHE */

			length_t mult_array[BASE] = {0};
			for (fang_t i = multiplier; i > 0; i /= BASE)
				mult_array[i % BASE] += 1;

			for (; multiplicand <= multiplicand_max; multiplicand += BASE - 1) {
				uint16_t product_array[BASE] = {0};
				for (vamp_t p = product; p > 0; p /= BASE)
					product_array[p % BASE] += 1;

				for (digit_t i = 0; i < BASE; i++)
					if (product_array[i] < mult_array[i])
						goto vampire_exit;

				digit_t temp;
				for (fang_t m = multiplicand; m > 0; m /= BASE) {
					temp = m % BASE;
					if (product_array[temp] == 0)
						goto vampire_exit;
					else
						product_array[temp]--;
				}
				for (digit_t i = 0; i < (BASE - 1); i++)
					if (product_array[i] != mult_array[i])
						goto vampire_exit;

				if (mult_zero || notrailingzero(multiplicand)) {
					vargs_iterate_local_count(args);
					vargs_print_results(product, multiplier, multiplicand);
					llnode_add(&(ll), product);
				}
vampire_exit:
				product += product_iterator;
			}

#endif /* CACHE */
		}
	}
	array_new(&(args->result), ll, &(args->local_count));
	llnode_free(ll);
	return;
}
