// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025  Pierro Zachareas
 */

#include <stdlib.h>
#include <string.h>

#include "configuration.h"
#include "configuration_adv.h"
#include "helper.h"
#include "llnode.h"

struct llnode
{
	void *data;
	size_t element_size;
	size_t logical_size; // The first unoccupied element.
	struct llnode *next;
};

void llnode_new(struct llnode **ptr, size_t element_size, struct llnode *next)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);

	if (ptr == NULL)
		return;

	struct llnode *new = malloc(sizeof(struct llnode));
	if (new == NULL)
		abort();

	new->data = malloc(element_size * LINK_SIZE);
	if (new->data == NULL && (element_size * LINK_SIZE > 0))
		abort();

	new->element_size = element_size;
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
		if (i->data != NULL)
			free(i->data);
		free(i);
	}
}

void llnode_add(struct llnode **ptr, void *value)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr != NULL);

	if (ptr == NULL)
		return;
	if (value == NULL)
		return;

	if ((*ptr)->logical_size >= LINK_SIZE) {
		struct llnode *new = NULL;
		llnode_new(&new, (*ptr)->element_size, *ptr);
		*ptr = new;
	}
	size_t offset = (*ptr)->logical_size * (*ptr)->element_size;
	void *destination = (*ptr)->data + offset;

	memcpy(destination, value, (*ptr)->element_size);
	(*ptr)->logical_size += 1;
}

void *llnode_pop(struct llnode **ptr)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr != NULL);

	if (*ptr == NULL)
		return NULL;

	struct llnode *ret = *ptr;
	*ptr = ret->next;
	ret->next = 0;
	return ret;
}

void *llnode_getdata(struct llnode *ptr)
{
	OPTIONAL_ASSERT(ptr != NULL);
	return (ptr->data);

}

size_t llnode_count_elements(struct llnode *ptr)
{
	if (ptr == NULL)
		return 0;

	size_t size = 0;
	for (struct llnode *i = ptr; i != NULL; i = i->next)
		size += i->logical_size;

	return size;
}

size_t llnode_count_bytes(struct llnode *ptr)
{
	return (llnode_count_elements(ptr) * ptr->element_size);
}

// vamp_t

typedef struct llnode llvamp_t;

void llvamp_new(struct llvamp_t **ptr, struct llvamp_t *next)
{
	llnode_new((struct llnode **)ptr, sizeof(vamp_t), (struct llnode *) next);
}

void llvamp_free(struct llvamp_t *node)
{
	llnode_free((struct llnode *)node);
}

void llvamp_add(struct llvamp_t **ptr, vamp_t value)
{
	llnode_add((struct llnode **)ptr, &value);
}

struct llvamp_t *llvamp_pop(struct llvamp_t **ptr)
{
	return (llnode_pop((struct llnode **) ptr));
}

vamp_t *llvamp_getdata(struct llvamp_t *ptr)
{
	return ((vamp_t *)llnode_getdata((struct llnode *) ptr));
}

size_t llvamp_count_elements(struct llvamp_t *ptr)
{
	return (llnode_count_elements((struct llnode *)ptr));
}
