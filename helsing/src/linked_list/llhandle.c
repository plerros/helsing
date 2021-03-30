// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>

#include "configuration.h"
#include "llist.h"
#include "llhandle.h"

struct llhandle *llhandle_init()
{
	struct llhandle *new = malloc(sizeof(struct llhandle));
	if (new == NULL)
		abort();

	reset_head(new);
	new->size = 0;
	return new;
}

void llhandle_free(struct llhandle *handle)
{
	free_head(handle);
	free(handle);
}

void llhandle_add(struct llhandle *handle, vamp_t value)
{
	if (handle == NULL)
		return;

	init_head(handle, value);
	handle->size += 1;
}

void llhandle_reset(struct llhandle *handle)
{
	free_head(handle);
	handle->size = 0;
}


#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)

void llhandle_print(struct llhandle *ptr, vamp_t count)
{
	llist_print(ptr->head, count);
}

#endif 