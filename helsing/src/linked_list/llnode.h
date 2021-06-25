// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_LLNODE_H
#define HELSING_LLNODE_H

#include "configuration_adv.h"

#ifdef STORE_RESULTS
struct llnode
{
	vamp_t *data;
	vamp_t logical_size; // The first unoccupied element.
	struct llnode *next;
};
void llnode_free(struct llnode *list);
void llnode_add(struct llnode **ptr, vamp_t value);
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
#endif /* STORE_RESULTS */
#endif /* HELSING_LLNODE_H */