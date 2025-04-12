// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"


#if VAMPIRE_NUMBER_OUTPUTS
#include <stdlib.h>
#include "llnode.h"

#if VAMPIRE_HASH
#include <openssl/evp.h>
#endif

static void llnode_new(struct llnode **ptr, struct llnode *next)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	struct llnode *new = malloc(sizeof(struct llnode));
	if (new == NULL)
		abort();

	new->data = malloc(sizeof(vamp_t) * LINK_SIZE);
	if (new->data == NULL)
		abort();

	new->logical_size = 0;
	new->next = next;
	*ptr = new;
}

void llnode_free(struct llnode *node)
{
	if (node == NULL)
		return;

	struct llnode *tmp = node;
	for (struct llnode *i = tmp; tmp != NULL; i = tmp) {
		tmp = tmp->next;
		free(i->data);
		free(i);
	}
}

void llnode_add(struct llnode **ptr, vamp_t value)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(value != 0);

	if (*ptr == NULL) {
		llnode_new(ptr, NULL);
	}
	else if ((*ptr)->logical_size >= LINK_SIZE) {
		struct llnode *new = NULL;
		llnode_new(&new, *ptr);
		*ptr = new;
	}
	(*ptr)->data[(*ptr)->logical_size] = value;
	(*ptr)->logical_size += 1;
}

vamp_t llnode_getsize(struct llnode *ptr)
{
	vamp_t size = 0;
	for (struct llnode *i = ptr; i != NULL; i = i->next)
		size += i->logical_size;

	return size;
}
#endif /* VAMPIRE_NUMBER_OUTPUTS */
