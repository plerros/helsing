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
#include "llhandle.h"
#include "bthandle.h"
#include "cache.h"
#include "vargs.h"

static bool notrailingzero(fang_t x)
{
	return ((x % 10) != 0);
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
	fang_t root = sqrtv_floor(x);
	if (root == fang_max)
		return root;

	return (x / root);
}

// Modulo 9 lack of congruence
static bool con9(vamp_t x, vamp_t y)
{
	return ((x + y) % 9 != (x * y) % 9);
}

void vargs_new(struct vargs **ptr, struct cache *digptr)
{
	if (ptr == NULL)
		return;

	struct vargs *new = malloc(sizeof(struct vargs));
	if (new == NULL)
		abort();

	new->digptr = digptr;
	new->local_count = 0;
	llhandle_new(&(new->lhandle));
	bthandle_new(&(new->thandle));
	*ptr = new;
}

void vargs_free(struct vargs *args)
{
	if (args == NULL)
		return;

	llhandle_free(args->lhandle);
	bthandle_free(args->thandle);
	free(args);
}

void vargs_reset(struct vargs *args)
{
	args->local_count = 0;
	llhandle_free(args->lhandle);
	llhandle_new(&(args->lhandle));
	bthandle_reset(args->thandle);
}

void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax)
{
	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

#if CACHE
	length_t length_max = length(max);
	length_t length_a = length_max / 3;
	if (length_max < 9)
		length_a += length_max % 3;

	fang_t power_a = pow10v(length_a);
	digits_t *dig = args->digptr->dig;
#endif

	for (fang_t multiplier = fmax; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		fang_t multiplicand = div_roof(min, multiplier); // fmin * fmax <= min - 10^n
		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier > max_sqrt)
			multiplicand_max = max / multiplier;
		else
			multiplicand_max = multiplier;
			// multiplicand <= multiplier: 5267275776 = 72576 * 72576.

		while (multiplicand <= multiplicand_max && con9(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand <= multiplicand_max) {
			vamp_t product_iterator = multiplier;
			product_iterator *= 9; // <= 9 * 2^32
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

			for (; multiplicand <= multiplicand_max; multiplicand += 9) {
				if (digd + dig[e0] + dig[e1] == dig[de0] + dig[de1] + dig[de2])
					if (mult_zero || notrailingzero(multiplicand)) {
						vargs_iterate_local_count(args);
						vargs_print_results(product, multiplier, multiplicand);
						bthandle_add(args->thandle, product);
					}
				product += product_iterator;
				e0 += 9;
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

			length_t mult_array[10] = {0};
			for (fang_t i = multiplier; i > 0; i /= 10)
				mult_array[i % 10] += 1;

			for (; multiplicand <= multiplicand_max; multiplicand += 9) {
				uint16_t product_array[10] = {0};
				for (vamp_t p = product; p > 0; p /= 10)
					product_array[p % 10] += 1;

				for (digit_t i = 0; i < 10; i++)
					if (product_array[i] < mult_array[i])
						goto vampire_exit;

				digit_t temp;
				for (fang_t m = multiplicand; m > 0; m /= 10) {
					temp = m % 10;
					if (product_array[temp] == 0)
						goto vampire_exit;
					else
						product_array[temp]--;
				}
				for (digit_t i = 0; i < 9; i++)
					if (product_array[i] != mult_array[i])
						goto vampire_exit;

				if (mult_zero || notrailingzero(multiplicand)) {
					vargs_iterate_local_count(args);
					vargs_print_results(product, multiplier, multiplicand);
					bthandle_add(args->thandle, product);
				}
vampire_exit:
				product += product_iterator;
			}

#endif /* CACHE */

			if (multiplier < max_sqrt && mult_zero)
				bthandle_cleanup(args->thandle, args->lhandle, product);
		}
	}
	bthandle_cleanup(args->thandle, args->lhandle, 0);
	llhandle_getfield_size(args->lhandle, &(args->local_count));
	return;
}