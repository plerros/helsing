// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_LLNODE_H
#define HELSING_LLNODE_H

#include "configuration_adv.h"
#include "hash.h"

#ifdef STORE_RESULTS
struct llnode
{
	vamp_t *data;
	vamp_t size;         // data[size]
	vamp_t logical_size; // The first unoccupied element.
	struct llnode *next;
};
void llnode_free(struct llnode *list);
void llnode_add(struct llnode **ptr, vamp_t value);
vamp_t llnode_sort(struct llnode **pptr);
#else /* STORE_RESULTS */
struct llnode
{
};
static inline void llnode_free(
	__attribute__((unused)) struct llnode *list)
{
}
static inline void llnode_add(
	__attribute__((unused)) struct llnode **ptr,
	__attribute__((unused)) vamp_t value)
{
}
static inline vamp_t llnode_sort(
	__attribute__((unused)) struct llnode **pptr)
{
	return 0;
}
#endif /* STORE_RESULTS */

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
void llnode_checksum(struct llnode *node, struct hash *checksum);
#else
static inline void llnode_checksum(
	__attribute__((unused)) struct llnode *list,
	__attribute__((unused)) struct hash *checksum)
{
}
#endif

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
void llnode_print(struct llnode *list, vamp_t count);
#else
static inline void llnode_print(
	__attribute__((unused)) struct llnode *list,
	__attribute__((unused)) vamp_t count)
{
}
#endif
#endif /* HELSING_LLNODE_H */