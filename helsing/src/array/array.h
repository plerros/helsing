// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_ARRAY_H
#define HELSING_ARRAY_H

#include "configuration_adv.h"
#include "llnode.h"
#include "hash.h"

#ifdef STORE_RESULTS
struct array
{
	vamp_t *number;
	vamp_t *fangs;
	vamp_t size;
};
void array_free(struct array *ptr);
#else  /* STORE_RESULTS */
struct array
{
};
static inline void array_free(__attribute__((unused)) struct array *ptr)
{
}
#endif /* STORE_RESULTS */

#ifdef PROCESS_RESULTS
void array_new(struct array **ptr, struct llnode *ll, vamp_t (*count_ptr)[MAX_FANG_PAIRS]);
#else  /* PROCESS_RESULTS */
static inline void array_new(
	__attribute__((unused)) struct array **ptr,
	__attribute__((unused)) struct llnode *ll,
	__attribute__((unused)) vamp_t (*count_ptr)[MAX_FANG_PAIRS])
{
}
#endif /* PROCESS_RESULTS */

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
void array_checksum(struct array *ptr, struct hash *checksum);
#else
static inline void array_checksum(
	__attribute__((unused)) struct array *ptr,
	__attribute__((unused)) struct hash *checksum)
{
}
#endif

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
void array_print(struct array *ptr, vamp_t count[MAX_FANG_PAIRS]);
#else
static inline void array_print(
	__attribute__((unused)) struct array *ptr,
	__attribute__((unused)) vamp_t count[MAX_FANG_PAIRS])
{
}
#endif
#endif /* HELSING_ARRAY_H */