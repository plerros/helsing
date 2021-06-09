// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2012 Jens Kruse Andersen
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#if CACHE
#include <stdlib.h>
#include "helper.h"
#include "cache.h"
#endif

#if CACHE

digits_t set_dig(fang_t number)
{
	digits_t ret = 0;
	digits_t tmp[10] = {0};
	for (; number > 0; number /= 10)
		tmp[number % 10] += 1;

	for (digit_t i = 1; i < 10; i++)
		ret = ret * DIG_BASE + tmp[i];

	return ret;
}

void cache_new(struct cache **ptr, vamp_t max)
{
	if (ptr == NULL)
		return;

	struct cache *new = malloc(sizeof(struct cache));
	if (new == NULL)
		abort();

	length_t length_a = length(max) / 3;
	length_t length_b = length(max) - (2 * length_a);
	new->size = pow10v(length_b);

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

#endif /* CACHE */