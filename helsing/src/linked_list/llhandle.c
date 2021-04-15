// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"

#ifdef PROCESS_RESULTS
#include <stdlib.h>
#include "llnode.h"
#include "llhandle.h"
#include "hash.h"
#endif

#ifdef PROCESS_RESULTS
void llhandle_new(struct llhandle **ptr)
{
	if (ptr == NULL)
		return;

	struct llhandle *new = malloc(sizeof(struct llhandle));
	if (new == NULL)
		abort();

	new->first = NULL;
	new->size = 0;
	*ptr = new;
}

void llhandle_free(struct llhandle *ptr)
{
	llnode_free(ptr->first);
	free(ptr);
}

void llhandle_add(struct llhandle *ptr, vamp_t value)
{
	if (ptr == NULL)
		return;

	llnode_add(&(ptr->first), value, ptr->first);
	ptr->size += 1;
}

void llhandle_reset(struct llhandle *ptr)
{
	llnode_free(ptr->first);
	ptr->first = NULL;
	ptr->size = 0;
}
#endif /* PROCESS_RESULTS */

#if defined(PROCESS_RESULTS) && defined(CHECKSUM_RESULTS)
void llhandle_checksum(struct llhandle *ptr, struct hash *checksum)
{
	llnode_checksum(ptr->first, checksum);
}
#endif

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void llhandle_print(struct llhandle *ptr, vamp_t count)
{
	llnode_print(ptr->first, count);
}
#endif