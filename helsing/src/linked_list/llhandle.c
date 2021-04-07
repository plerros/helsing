// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>

#include "configuration.h"
#include "llist.h"
#include "llhandle.h"

#ifdef PROCESS_RESULTS
void llhandle_init(struct llhandle **ptr)
{
	struct llhandle *new = malloc(sizeof(struct llhandle));
	if (new == NULL)
		abort();

	new->head = NULL;
	new->size = 0;
	*ptr = new;
}

void llhandle_free(struct llhandle *ptr)
{
	llist_free(ptr->head);
	free(ptr);
}

void llhandle_add(struct llhandle *ptr, vamp_t value)
{
	if (ptr == NULL)
		return;

	llist_init(&(ptr->head), value, ptr->head);
	ptr->size += 1;
}

void llhandle_reset(struct llhandle *ptr)
{
	llist_free(ptr->head);
	ptr->head = NULL;
	ptr->size = 0;
}
#endif /* PROCESS_RESULTS */

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)

void llhandle_print(struct llhandle *ptr, vamp_t count)
{
	llist_print(ptr->head, count);
}

#endif