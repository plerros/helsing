// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_BTREE_H
#define HELSING_BTREE_H

#include "configuration.h"

#ifdef PROCESS_RESULTS
#include "llhandle.h"
#else
#include <stddef.h> // NULL
#endif


#ifdef PROCESS_RESULTS
struct btree;
struct btree *btree_init(vamp_t value);
void btree_free(struct btree *tree);
struct btree *btree_add(
	struct btree *tree,
	vamp_t node,
	vamp_t *count);
struct btree *btree_cleanup(
	struct btree *tree,
	vamp_t number,
	struct llhandle *lhandle,
	vamp_t *btree_size);
#else /* PROCESS RESULTS */
struct btree
{
};
static inline struct btree *btree_init(__attribute__((unused)) vamp_t value)
{
	return NULL;
}
static inline void btree_free(__attribute__((unused)) struct btree *tree)
{
}
static inline struct btree *btree_add(
	__attribute__((unused)) struct btree *tree,
	__attribute__((unused)) vamp_t node,
	__attribute__((unused)) vamp_t *count)
{
	return NULL;
}
struct btree *btree_cleanup(
	__attribute__((unused)) struct btree *tree,
	__attribute__((unused)) vamp_t number,
	__attribute__((unused)) struct llhandle *lhandle,
	__attribute__((unused)) vamp_t *btree_size)
{
	return NULL;
}
#endif /* PROCESS RESULTS */
#endif /* HELSING_BTREE_H */