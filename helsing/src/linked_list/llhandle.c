// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#ifdef PROCESS_RESULTS
#include <stdlib.h>
#include <string.h>
#include "llnode.h"
#include "llhandle.h"
#include "hash.h"
#endif

#if defined(PROCESS_RESULTS) && SANITY_CHECK
	#include <assert.h>
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
	if (ptr == NULL)
		return;

	llnode_free(ptr->first);
	free(ptr);
}

void llhandle_add(struct llhandle *ptr, vamp_t value)
{
#if SANITY_CHECK
	assert(ptr != NULL);
#endif
	llnode_add(&(ptr->first), value, ptr->first);
	ptr->size += 1;
}

void llhandle_reset(struct llhandle *ptr)
{
	llnode_free(ptr->first);
	ptr->first = NULL;
	ptr->size = 0;
}

void swap(vamp_t num1, vamp_t num2, struct llnode *node) {

#if SANITY_CHECK
	assert(node->current > 0);
#endif
	vamp_t max = node->current - 1;

#if SANITY_CHECK
	assert(max >= num1);
	assert(max >= num2);
#endif
	vamp_t temp = node->value[max - num1];
	node->value[max - num1] = node->value[max - num2];
	node->value[max - num2] = temp;
}

vamp_t partition(vamp_t lo, vamp_t hi, struct llnode *node) {
#if SANITY_CHECK
	assert(node->current > 0);
#endif
	vamp_t max = node->current - 1;

#if SANITY_CHECK
	assert(max >= hi);
#endif
	vamp_t pivot = node->value[max - hi];

	vamp_t i = lo;
	for (vamp_t j = lo; j <= hi; j++) {
		if (node->value[max - j] < pivot) {
			swap(i, j, node);
			i++;
		}
	}
	swap(i, hi, node);
	return i;
}

void quickSort(vamp_t lo, vamp_t hi, struct llnode *node) {
	if(hi <= lo)
		return;

	vamp_t partitionPoint = partition(lo, hi, node);
	if (partitionPoint > 0)
		quickSort(lo, partitionPoint-1, node);
	if (partitionPoint < vamp_max)
		quickSort(partitionPoint+1, hi, node);
}

void llhandle_sort(struct llhandle *ptr) {
	if (ptr->first == NULL)
		return;

	struct llnode *tmp;
	llnode_new(&(tmp), ptr->size, NULL);

	vamp_t x = 0;
	for (struct llnode *i = ptr->first; i != NULL; i = i->next) {
		memcpy(&(tmp->value[x]), &(i->value[0]), (i->current) * sizeof(vamp_t));
		x += i->current;

		tmp->current += i->current;
	}

	llnode_free(ptr->first);
	quickSort(0, tmp->current - 1, tmp);

	ptr->size = 0;

	for (vamp_t j = 0; j < tmp->current; j++) {
		vamp_t numcount = 1;

		for (vamp_t curr = tmp->value[j]; j+1 < tmp->current && tmp->value[j+1] == curr; j++) {
			tmp->value[j] = 0;
			numcount++;
		}
		if (numcount < MIN_FANG_PAIRS)
			tmp->value[j] = 0;
		else
			ptr->size += 1;
	}

	ptr->first = tmp;
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