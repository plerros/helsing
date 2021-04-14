// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_LLNODE_H
#define HELSING_LLNODE_H

#include "configuration.h"
#include "hash.h"

#ifdef STORE_RESULTS
struct llnode
{
	vamp_t value[LINK_SIZE];
	uint16_t current;
	struct llnode *next;
};
void llnode_init(struct llnode **ptr, vamp_t value, struct llnode *next);
void llnode_free(struct llnode *list);
#else /* STORE_RESULTS */
struct llnode
{
};
static inline void llnode_init(
	__attribute__((unused)) struct llnode **ptr,
	__attribute__((unused)) vamp_t value,
	__attribute__((unused)) struct llnode *next)
{
}
static inline void llnode_free(
	__attribute__((unused)) struct llnode *list)
{
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