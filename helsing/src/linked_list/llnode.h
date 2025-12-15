// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_LLNODE_H
#define HELSING_LLNODE_H

#include "configuration_adv.h"

struct llnode
{
	void *data;
	size_t element_size;
	size_t logical_size; // The first unoccupied element.
	struct llnode *next;
};

void llnode_new(struct llnode **ptr, size_t element_size, struct llnode *next);
void llnode_free(struct llnode *list);
void llnode_add(struct llnode **ptr, void *value);
size_t llnode_getsize(struct llnode *ptr);
#endif /* HELSING_LLNODE_H */
