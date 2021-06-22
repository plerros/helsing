// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#ifdef STORE_RESULTS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "llnode.h"
#include "hash.h"
#endif

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
#include <openssl/evp.h>
#endif

#if defined(STORE_RESULTS) && SANITY_CHECK
	#include <assert.h>
#endif

#ifdef STORE_RESULTS

static void llnode_new(struct llnode **ptr, vamp_t size, struct llnode *next)
{
	if (ptr == NULL)
		return;

	struct llnode *new = malloc(sizeof(struct llnode));
	if (new == NULL)
		abort();

	new->data = malloc(sizeof(vamp_t) * size);
	if (new->data == NULL)
		abort();

	new->size = size;
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
		llnode_new(ptr, LINK_SIZE, NULL);
	}
	else if ((*ptr)->logical_size >= (*ptr)->size) {
		struct llnode *new;
		llnode_new(&new, LINK_SIZE, *ptr);
		*ptr = new;
	}
	(*ptr)->data[(*ptr)->logical_size] = value;
	(*ptr)->logical_size += 1;
}

static void llnode_concat(struct llnode **ptr)
{
#if SANITY_CHECK
	assert(ptr != NULL);
#endif
	vamp_t size = 0;
	for (struct llnode *i = (*ptr); i != NULL; i = i->next)
		size += i->logical_size;

	struct llnode *new;
	llnode_new(&(new), size, NULL);

	for (struct llnode *i = (*ptr); i != NULL; i = i->next) {
		memcpy(&(new->data[new->logical_size]), i->data, (i->logical_size) * sizeof(vamp_t));
		new->logical_size += i->logical_size;
	}

	llnode_free(*ptr);
	*ptr = new;
}

static void swap(vamp_t x, vamp_t y, vamp_t *arr)
{
	vamp_t tmp = arr[x];
	arr[x] = arr[y];
	arr[y] = tmp;
}

static vamp_t partition(vamp_t lo, vamp_t hi, vamp_t *arr)
{
	vamp_t pivot = arr[hi];
	vamp_t i = lo;
	for (vamp_t j = lo; j <= hi; j++) {
		if (arr[j] < pivot) {
			swap(i, j, arr);
			i++;
		}
	}
	swap(i, hi, arr);
	return i;
}

static void quickSort(vamp_t lo, vamp_t hi, vamp_t *arr)
{
	if (hi <= lo)
		return;

	vamp_t partitionPoint = partition(lo, hi, arr);
	if (partitionPoint > 0)
		quickSort(lo, partitionPoint-1, arr);
	if (partitionPoint < vamp_max)
		quickSort(partitionPoint+1, hi, arr);
}

vamp_t llnode_sort(struct llnode **ptr)
{
	if (*ptr == NULL)
		return 0;

	llnode_concat(ptr);

	vamp_t *arr = (*ptr)->data;
	vamp_t arr_size = (*ptr)->logical_size;

	if (arr_size > 0)
		quickSort(0, arr_size - 1, arr);

	vamp_t count = 0;

	for (vamp_t j = 0; j < arr_size; j++) {
		if (arr[j] == 0)
			continue;

		vamp_t value = arr[j];
		vamp_t fang_pairs = 1;

		for (; j+1 < arr_size && arr[j+1] == value; j++) {
			arr[j] = 0;
			fang_pairs++;
		}
		if (fang_pairs < MIN_FANG_PAIRS)
			arr[j] = 0;
		else
			count += 1;
	}
	return count;
}

#endif /* STORE_RESULTS */

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
void llnode_checksum(struct llnode *node, struct hash *checksum)
{
	for (struct llnode *i = node; i != NULL; i = i->next) {
		for (vamp_t j = 0; j < i->logical_size; j++) {
			if (i->data[j] == 0)
				continue;

			vamp_t tmp = i->data[j];

			#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
			tmp = __builtin_bswap64(tmp);
			#endif

			EVP_DigestInit_ex(checksum->mdctx, checksum->md, NULL);

			EVP_DigestUpdate(checksum->mdctx, checksum->md_value, checksum->md_size);
			EVP_DigestUpdate(checksum->mdctx, &tmp, sizeof(tmp));

			EVP_DigestFinal_ex(checksum->mdctx, checksum->md_value, NULL);
		}
	}
}
#endif

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
void llnode_print(struct llnode *node, vamp_t count)
{
	for (struct llnode *i = node; i != NULL; i = i->next) {
		for (vamp_t j = 0; j < i->logical_size; j++) {
			if (i->data[j] == 0)
				continue;

			fprintf(stdout, "%llu %llu\n", ++count, i->data[j]);
			fflush(stdout);
		}
	}
}
#endif