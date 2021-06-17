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
//	assert(node->current > 0);
	vamp_t max = node->current - 1;

//	assert(max >= num1);
//	assert(max >= num2);
	vamp_t temp = node->value[max - num1];
	node->value[max - num1] = node->value[max - num2];
	node->value[max - num2] = temp;
}

vamp_t partition(vamp_t lo, vamp_t hi, struct llnode *node) {
//	assert(node->current > 0);
	vamp_t max = node->current - 1;

//	assert(max >= hi);
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

	struct llnode *tmpnode;
	llnode_new(&(tmpnode), ptr->size, NULL);

	vamp_t x = 0;
	for (struct llnode *i = ptr->first; i != NULL; i = i->next) {
		memcpy(&(tmpnode->value[x]), &(i->value[0]), (i->current) * sizeof(vamp_t));
		x += i->current;

		tmpnode->current += i->current;
	}

	llnode_free(ptr->first);
	quickSort(0, tmpnode->current - 1, tmpnode);

	vamp_t total = 0;

	struct llnode *i = tmpnode;

	for (vamp_t j = 0; j < i->current; j++) {
		vamp_t numcount = 1;
		vamp_t curr = i->value[j];

		for (; j+1 < i->current && i->value[j+1] == curr; j++) {
			i->value[j] = 0;
			numcount++;
		}
		if (numcount < MIN_FANG_PAIRS) {
			i->value[j] = 0;
		} else {
			total++;
		}
	}

	ptr->first = tmpnode;
	ptr->size = total;
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