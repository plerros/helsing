// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021 Pierro Zachareas
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

static digits_t digbase()
{
#if (BASE == 2)
	return 0;
#else
	return(pow(2.0, ((double)ELEMENT_BITS)/(double)(BASE - 1)));
#endif
}

digits_t set_dig(fang_t number)
{
	digits_t ret = 0;
	digits_t tmp[BASE] = {0};
	for (; number > 0; number /= BASE)
		tmp[number % BASE] += 1;

	for (digit_t i = 1; i < BASE; i++)
		ret = ret * digbase() + tmp[i];

	return ret;
}

void cache_new(struct cache **ptr, vamp_t max)
{
#if SANITY_CHECK
	assert(ptr != NULL);
	assert (*ptr == NULL);
#endif

	struct cache *new = malloc(sizeof(struct cache));
	if (new == NULL)
		abort();

	length_t length_a = length(max) / 3;
	length_t length_b = length(max) - (2 * length_a);
	new->size = pow_v(length_b);

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
	if (log(max) / log(BASE - 1) <= digbase())
		return false;
#else
	if (log2(max) <= ELEMENT_BITS)
		return false;
#endif

	return true;
}

#endif /* CACHE */