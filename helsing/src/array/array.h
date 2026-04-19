// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_ARRAY_H
#define HELSING_ARRAY_H

#include <threads.h>

#include "output.h"

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
	int unused;
};
static inline void array_free(ATTR_UNUSED struct array *ptr)
{
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (defined STORE_RESULTS) */

#if VAMPIRE_NUMBER_OUTPUTS
void array_new(struct array **ptr, struct llvamp_t **ll, vamp_t (*count_ptr)[COUNT_ARRAY_SIZE]);
#else
static inline void array_new(
	ATTR_UNUSED struct array **ptr,
	ATTR_UNUSED struct llvamp_t **ll,
	ATTR_UNUSED vamp_t (*count_ptr)[COUNT_ARRAY_SIZE])
{
}
#endif /* VAMPIRE_NUMBER_OUTPUTS */

#if (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH)
void array_checksum(struct array *ptr, struct hash *checksum);
#else
static inline void array_checksum(
	ATTR_UNUSED struct array *ptr,
	ATTR_UNUSED struct hash *checksum)
{
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH) */

#if (VAMPIRE_NUMBER_OUTPUTS) && (defined PRINT_RESULTS)
void array_print(
	struct array *ptr,
	mtx_t *stdout_mtx,
	vamp_t count[COUNT_ARRAY_SIZE],
	vamp_t (*prev)[COUNT_ARRAY_SIZE]);
#else
static inline void array_print(
	ATTR_UNUSED struct array *ptr,
	ATTR_UNUSED mtx_t *stdout_mtx,
	ATTR_UNUSED vamp_t count[COUNT_ARRAY_SIZE],
	ATTR_UNUSED vamp_t (*prev)[COUNT_ARRAY_SIZE])
{
}
#endif /* (VAMPIRE_NUMBER_OUTPUTS) && (defined PRINT_RESULTS) */
#endif /* HELSING_ARRAY_H */
