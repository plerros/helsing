// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_BTNODE_H
#define HELSING_BTNODE_H

#include "configuration.h"

#ifdef PROCESS_RESULTS
#include "llhandle.h"
#else
#include <stddef.h> // NULL
#endif

#ifdef PROCESS_RESULTS
struct btnode
{
	struct btnode *left;
	struct btnode *right;
	vamp_t key;
	length_t height; //Should probably be less than 32
	uint8_t fang_pairs;
};
void btnode_init(struct btnode **ptr, vamp_t key);
void btnode_free(struct btnode *node);
struct btnode *btnode_add(
	struct btnode *node,
	vamp_t key,
	vamp_t *count);
struct btnode *btnode_cleanup(
	struct btnode *node,
	vamp_t key,
	struct llhandle *lhandle,
	vamp_t *btnode_size);
#else /* PROCESS RESULTS */
struct btnode
{
};
static inline void btnode_init(
	__attribute__((unused)) struct btnode **ptr,
	__attribute__((unused)) vamp_t key)
{
}
static inline void btnode_free(__attribute__((unused)) struct btnode *node)
{
}
static inline struct btnode *btnode_add(
	__attribute__((unused)) struct btnode *node,
	__attribute__((unused)) vamp_t key,
	__attribute__((unused)) vamp_t *count)
{
	return NULL;
}
struct btnode *btnode_cleanup(
	__attribute__((unused)) struct btnode *node,
	__attribute__((unused)) vamp_t key,
	__attribute__((unused)) struct llhandle *lhandle,
	__attribute__((unused)) vamp_t *btnode_size)
{
	return NULL;
}
#endif /* PROCESS RESULTS */
#endif /* HELSING_BTNODE_H */