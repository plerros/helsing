// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "configuration.h"
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

struct vargs *vargs_init(struct cache *digptr)
{
	struct vargs *new = malloc(sizeof(struct vargs));
	if (new == NULL)
		abort();

	new->local_count = 0;
	new->digptr = digptr;
	vargs_init_lhandle(new);
	vargs_init_thandle(new);
	vargs_init_total(new);
	return new;
}

void vargs_free(struct vargs *args)
{
	vargs_free_lhandle(args);
	vargs_free_thandle(args);
	free (args);
}

void vargs_reset(struct vargs *args)
{
	args->local_count = 0;
	vargs_init_lhandle(args);

#ifdef PROCESS_RESULTS
	bthandle_reset(args->thandle);
#endif
}

struct llhandle *vargs_getlhandle(__attribute__((unused)) struct vargs *args)
{
	struct llhandle *ret = NULL;
#ifdef PROCESS_RESULTS
	ret = args->lhandle;
	args->lhandle = NULL;
#endif
	return ret;
}

#ifdef PROCESS_RESULTS

void vargs_btree_cleanup(
	__attribute__((unused)) struct vargs *args,
	__attribute__((unused)) vamp_t number)
{
	bthandle_cleanup(args->thandle, args->lhandle, number);
}

#endif

#if !CACHE

void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax)
{
	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

	for (fang_t multiplier = fmax; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		fang_t multiplicand = div_roof(min, multiplier); // fmin * fmax <= min - 10^n
		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier >= max_sqrt)
			multiplicand_max = max / multiplier;
		else
			multiplicand_max = multiplier;
			// multiplicand can be equal to multiplier:
			// 5267275776 = 72576 * 72576.

		while (multiplicand <= multiplicand_max && con9(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand <= multiplicand_max) {
			vamp_t product_iterator = multiplier;
			product_iterator *= 9; // <= 9 * 2^32
			vamp_t product = multiplier;
			product *= multiplicand; // avoid overflow

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
					#if defined COUNT_RESULTS ||  defined DUMP_RESULTS
						args->local_count += 1;
					#endif
					#ifdef DUMP_RESULTS
						printf("%llu = %lu x %lu\n", product, multiplier, multiplicand);
					#endif
					#ifdef PROCESS_RESULTS
						bthandle_add(args->thandle, product);
					#endif
				}
vampire_exit:
				product += product_iterator;
			}
			if (multiplier < max_sqrt && mult_zero)
				vargs_btree_cleanup(args, product);
		}
	}
	vargs_btree_cleanup(args, 0);
	#if MEASURE_RUNTIME
		#ifdef PROCESS_RESULTS
			args->total += args->lhandle->size;
		#elif defined COUNT_RESULTS ||  defined DUMP_RESULTS
			args->total += args->local_count;
		#endif
	#endif
	return;
}

#else /* !CACHE */

void vampire(vamp_t min, vamp_t max, struct vargs *args, fang_t fmax)
{
	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

	fang_t power_a = args->digptr->power_a;
	digits_t *dig = args->digptr->dig;

	for (fang_t multiplier = fmax; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		fang_t multiplicand = div_roof(min, multiplier); // fmin * fmax <= min - 10^n
		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier >= max_sqrt)
			multiplicand_max = max / multiplier;
		else
			multiplicand_max = multiplier;
			// multiplicand can be equal to multiplier:
			// 5267275776 = 72576 * 72576.

		while (multiplicand <= multiplicand_max && con9(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand <= multiplicand_max) {
			vamp_t product_iterator = multiplier;
			product_iterator *= 9; // <= 9 * 2^32
			vamp_t product = multiplier;
			product *= multiplicand; // avoid overflow

			fang_t step0 = product_iterator % power_a;
			fang_t step1 = product_iterator / power_a;

			fang_t e0 = multiplicand % power_a;
			fang_t e1 = multiplicand / power_a;

			/*
			 * digd = dig[multiplier];
			 * Each digd is calculated and accessed only once, we don't need to store them in memory.
			 * We can calculate digd on the spot and make the dig array 10 times smaller.
			 */

			digits_t digd;

			if (min_sqrt >= args->digptr->size)
				digd = set_dig(multiplier);
			else
				digd = dig[multiplier];

			fang_t de0 = product % power_a;
			fang_t de1 = (product / power_a) % power_a;
			fang_t de2 = ((product / power_a) / power_a);

			for (; multiplicand <= multiplicand_max; multiplicand += 9) {
				if (digd + dig[e0] + dig[e1] == dig[de0] + dig[de1] + dig[de2])
					if (mult_zero || notrailingzero(multiplicand)) {
					#if defined COUNT_RESULTS ||  defined DUMP_RESULTS
						args->local_count += 1;
					#endif
					#ifdef DUMP_RESULTS
						printf("%llu = %lu x %lu\n", product, multiplier, multiplicand);
					#endif
					#ifdef PROCESS_RESULTS
						bthandle_add(args->thandle, product);
					#endif
					}
				e0 += 9;
				if (e0 >= power_a) {
					e0 -= power_a;
					e1 ++;
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
				product += product_iterator;
			}
			if (multiplier < max_sqrt && mult_zero)
				vargs_btree_cleanup(args, product);
		}
	}
	vargs_btree_cleanup(args, 0);
	#if MEASURE_RUNTIME
		#ifdef PROCESS_RESULTS
			args->total += args->lhandle->size;
		#elif defined COUNT_RESULTS ||  defined DUMP_RESULTS
			args->total += args->local_count;
		#endif
	#endif
	return;
}

#endif  /* !CACHE */