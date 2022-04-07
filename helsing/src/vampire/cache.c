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

#if defined(CACHE) && SANITY_CHECK
#include <assert.h>
#endif

#if CACHE

#define DIGBASE(bits) ((vamp_t) pow(2.0, ((double)(bits))/(double)(BASE - 1)))

digits_t set_dig(fang_t number)
{
	digits_t ret = 0;
	digits_t tmp[BASE] = {0};
	for (; number > 0; number /= BASE)
		tmp[number % BASE] += 1;

	for (digit_t i = 1; i < BASE; i++)
		ret = ret * DIGBASE(ACTIVE_BITS) + tmp[i];

	return ret;
}

void cache_new(struct cache **ptr, vamp_t min, vamp_t max)
{
#if SANITY_CHECK
	assert(ptr != NULL);
	assert (*ptr == NULL);
#endif

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
	} while (i < length(max));
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
#if (BASE != 2) // avoid division by 0
	if (log(max) / log(BASE - 1) <= DIGBASE(COMPARISON_BITS))
		return false;
#else
	if (log2(max) <= COMPARISON_BITS) // Always true, unless COMPARISON_BITS is 32
		return false;
#endif

	return true;
}

#endif /* CACHE */
