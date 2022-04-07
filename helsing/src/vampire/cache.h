// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_CACHE_H
#define HELSING_CACHE_H

#include "configuration.h"
#include "configuration_adv.h"
#include <stdbool.h>

#if CACHE
struct cache
{
	digits_t *dig;
	fang_t size;
};
digits_t set_dig(fang_t number);
void cache_new(struct cache **ptr, vamp_t min, vamp_t max);
void cache_free(struct cache *ptr);
bool cache_ovf_chk(vamp_t max);
#else /* !CACHE */
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
#endif /* CACHE */
#endif /* HELSING_CACHE_H */
