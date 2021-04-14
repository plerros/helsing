// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"

#ifdef PROCESS_RESULTS
#include <stdlib.h>
#include <assert.h>
#include "llhandle.h"
#include "btnode.h"
#include "bthandle.h"
#endif

#ifdef PROCESS_RESULTS
void bthandle_init(struct bthandle **ptr)
{
	struct bthandle *new = NULL;
	new = malloc(sizeof(struct bthandle));
	if (new == NULL)
		abort();

	new->node = NULL;
	new->size = 0;
	*ptr = new;
}

void bthandle_free(struct bthandle *handle)
{
	if (handle != NULL)
		btnode_free(handle->node);
	free(handle);
}

void bthandle_add(
	struct bthandle *handle,
	vamp_t key)
{
#if SANITY_CHECK
	assert(handle != NULL);
#endif
	handle->node = btnode_add(handle->node, key, &(handle->size));
}

void bthandle_reset(struct bthandle *handle)
{
	btnode_free(handle->node);
	handle->node = NULL;
	handle->size = 0;
}

/*
 * Move inactive data from binary tree to linked list
 * and free up memory. Works best with low thread counts.
 */
void bthandle_cleanup(
	struct bthandle *handle,
	struct llhandle *lhandle,
	vamp_t key)
{
	struct btnode *tree = handle->node;
	vamp_t *size = &(handle->size);
	handle->node = btnode_cleanup(tree, key, lhandle, size);
}

#endif /* PROCESS_RESULTS */