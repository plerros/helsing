// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021-2022 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#if CACHE
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "helper.h"
#include "cache.h"
#endif

#if CACHE

#define BITS_PER_NUMERAL(bits) ((double)(bits))/(double)(BASE - 1)

#if DEDICATED_BITFIELDS
#define BITS_PER_NUMERAL2(bits) floor(BITS_PER_NUMERAL(bits))
#else
#define BITS_PER_NUMERAL2(bits) BITS_PER_NUMERAL(bits)
#endif

#define DIGBASE(bits) ((vamp_t) pow(2.0, BITS_PER_NUMERAL2(bits)))

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
		length_t part_A = partition3(i);
		length_t part_B = 0;
		if (i / 2 > part_A)
			part_B = i - 2 * part_A;

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
	return (numeral_max >= (DIGBASE(ELEMENT_BITS) - 1) * (COMPARISON_BITS / ELEMENT_BITS));
}

#endif /* CACHE */
