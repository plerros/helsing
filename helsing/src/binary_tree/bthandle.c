// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <assert.h>

#include "configuration.h"

#ifdef PROCESS_RESULTS

#include "llhandle.h"
#include "btree.h"
#include "bthandle.h"

struct bthandle *bthandle_init()
{
	struct bthandle *new = NULL;
	new = malloc(sizeof(struct bthandle));
	if (new == NULL)
		abort();

	new->tree = NULL;
	new->size = 0;
	return new;
}

void bthandle_free(struct bthandle *handle)
{
	if (handle != NULL)
		btree_free(handle->tree);
	free(handle);
}

void bthandle_add(
	struct bthandle *handle,
	vamp_t number)
{
	#if SANITY_CHECK
		assert(handle != NULL);
	#endif
	handle->tree = btree_add(handle->tree, number, &(handle->size));
}

void bthandle_reset(struct bthandle *handle)
{
	btree_free(handle->tree);
	handle->tree = NULL;
	handle->size = 0;
}

/*
 * Move inactive data from binary tree to linked list
 * and free up memory. Works best with low thread counts.
 */
void bthandle_cleanup(
	struct bthandle *handle,
	struct llhandle *lhandle,
	vamp_t number)
{
	struct btree *tree = handle->tree;
	vamp_t *size = &(handle->size);
	handle->tree = btree_cleanup(tree, number, lhandle, size);
}

#endif /* PROCESS_RESULTS */