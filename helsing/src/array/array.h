// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_ARRAY_H
#define HELSING_ARRAY_H

#include "configuration_adv.h"
#include "llnode.h"
#include "hash.h"

#if (VAMPIRE_NUMBER_OUTPUTS) && (defined STORE_RESULTS)
struct array
{
	vamp_t *number;
	vamp_t *fangs;
	size_t size;
};
void array_free(struct array *ptr);
#else
struct array
{
};
static inline void array_free(__attribute__((unused)) struct array *ptr)
{
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (defined STORE_RESULTS) */

#if VAMPIRE_NUMBER_OUTPUTS
void array_new(struct array **ptr, struct llvamp_t **ll, vamp_t (*count_ptr)[COUNT_ARRAY_SIZE]);
#else 
static inline void array_new(
	__attribute__((unused)) struct array **ptr,
	__attribute__((unused)) struct llvamp_t **ll,
	__attribute__((unused)) vamp_t (*count_ptr)[COUNT_ARRAY_SIZE])
{
}
#endif /* VAMPIRE_NUMBER_OUTPUTS */

#if (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH)
void array_checksum(struct array *ptr, struct hash *checksum);
#else
static inline void array_checksum(
	__attribute__((unused)) struct array *ptr,
	__attribute__((unused)) struct hash *checksum)
{
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH) */

#if (VAMPIRE_NUMBER_OUTPUTS) && (defined PRINT_RESULTS)
void array_print(
	struct array *ptr,
	vamp_t count[COUNT_ARRAY_SIZE],
	vamp_t (*prev)[COUNT_ARRAY_SIZE]);
#else
static inline void array_print(
	__attribute__((unused)) struct array *ptr,
	__attribute__((unused)) vamp_t count[COUNT_ARRAY_SIZE],
	__attribute__((unused)) vamp_t (*prev)[COUNT_ARRAY_SIZE])
{
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (defined PRINT_RESULTS) */
#endif /* HELSING_ARRAY_H */
