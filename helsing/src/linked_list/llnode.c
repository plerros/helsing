// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#ifdef STORE_RESULTS
#include <stdlib.h>
#include "llnode.h"
#endif

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
#include <openssl/evp.h>
#endif

#if defined(STORE_RESULTS) && SANITY_CHECK
	#include <assert.h>
#endif

#ifdef STORE_RESULTS

static void llnode_new(struct llnode **ptr, struct llnode *next)
{
#if SANITY_CHECK
	assert(ptr != NULL);
#endif

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
#if SANITY_CHECK
	assert(ptr != NULL);
#endif
	if (*ptr == NULL) {
		llnode_new(ptr, NULL);
	}
	else if ((*ptr)->logical_size >= LINK_SIZE) {
		struct llnode *new;
		llnode_new(&new, *ptr);
		*ptr = new;
	}
	(*ptr)->data[(*ptr)->logical_size] = value;
	(*ptr)->logical_size += 1;
}
#endif /* STORE_RESULTS */