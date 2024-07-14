// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#include "helper.h"

#ifdef PROCESS_RESULTS
#include <stdlib.h>
#include <string.h>
#include "array.h"
#include "llnode.h"
#endif

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
#include <openssl/evp.h>
#include "hash.h"
#endif

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
#include <stdio.h>
#endif

#ifdef STORE_RESULTS
void array_free(struct array *ptr)
{
	if (ptr == NULL)
		return;

	free(ptr->number);
	free(ptr->fangs);
	free(ptr);
}
#endif

#ifdef PROCESS_RESULTS
int cmpvampt(const void *a, const void *b)
{
	vamp_t tmp_a = (*(vamp_t *) a);
	vamp_t tmp_b = (*(vamp_t *) b);

	if (tmp_a > tmp_b)
		return 1;
	else if (tmp_a < tmp_b)
		return -1;
	else
		return 0;
}

void array_new(
	struct array **ptr,
	struct llnode *ll,
	vamp_t (*count_ptr)[MAX_FANG_PAIRS])
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(*ptr == NULL);
	OPTIONAL_ASSERT(count_ptr != NULL);

	if (ll == NULL)
		return;

	vamp_t size = llnode_getsize(ll);
	if (size == 0)
		return;

	vamp_t *number = malloc(sizeof(vamp_t) * size);
	if (number == NULL)
		abort();

	vamp_t *fangs = malloc(sizeof(vamp_t) * size);
	if (fangs == NULL)
		abort();

	// copy
	for (vamp_t i = 0; ll != NULL; ll = ll->next) {
		memcpy(&(number[i]), ll->data, (ll->logical_size) * sizeof(vamp_t));
		i += ll->logical_size;
	}

	// sort
	qsort(number, size, sizeof(vamp_t), cmpvampt);

	// filter fangs & resize
	vamp_t count[MAX_FANG_PAIRS];
	memset(count, 0, MAX_FANG_PAIRS * sizeof(vamp_t));

	// Initialize the fangs array
	for (vamp_t i = 0; i < size; i++)
		fangs[i] = 1;


	// Combine duplicate entries
	for (vamp_t i = 1; i < size; i++) {
		if (number[i - 1] != number[i])
			continue;

		fangs[i] += fangs[i - 1];
		fangs[i - 1] = 0;
		number[i - 1] = 0;

		if (fangs[i] > MAX_FANG_PAIRS)
			fangs[i] = MAX_FANG_PAIRS;
	}

	// Filter out with MIN_FANG_PAIRS & count the results
	for (vamp_t i = 0; i < size; i++) {
		if (fangs[i] < MIN_FANG_PAIRS) {
			number[i] = 0;
			fangs[i] = 0;
			continue;
		}
		for (vamp_t j = MIN_FANG_PAIRS - 1; j < fangs[i]; j++)
			count[j]++;
	}

#ifdef STORE_RESULTS
	struct array *new = malloc(sizeof(struct array));
	if (new == NULL)
		abort();

	new->number = number;
	new->fangs = fangs;
	new->size = size;
	*ptr = new;
#else
	free(number);
	free(fangs);
	*ptr = NULL;
#endif
	memcpy(count_ptr, count, MAX_FANG_PAIRS * sizeof(vamp_t));
	return;
}
#endif /* PROCESS_RESULTS */

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
void array_checksum(struct array *ptr, struct hash *checksum)
{
	OPTIONAL_ASSERT(ptr != NULL);
	OPTIONAL_ASSERT(checksum != NULL);

	for (vamp_t i = 0; i < ptr->size; i++) {
		if (ptr->number[i] == 0)
			continue;

		vamp_t tmp = ptr->number[i];

		#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
		tmp = __builtin_bswap64(tmp);
		#endif

		EVP_DigestInit_ex(checksum->mdctx, checksum->md, NULL);

		EVP_DigestUpdate(checksum->mdctx, checksum->md_value, checksum->md_size);
		EVP_DigestUpdate(checksum->mdctx, &tmp, sizeof(tmp));

		EVP_DigestFinal_ex(checksum->mdctx, checksum->md_value, NULL);
	}
}
#endif

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
void array_print(struct array *ptr, vamp_t count[MAX_FANG_PAIRS])
{
	OPTIONAL_ASSERT(ptr != NULL);

	vamp_t local_count[MAX_FANG_PAIRS];
	memcpy(local_count, count, MAX_FANG_PAIRS * sizeof(vamp_t));

	for (vamp_t i = 0; i < ptr->size; i++) {
		if (ptr->number[i] == 0)
			continue;

		for (size_t j = MIN_FANG_PAIRS - 1; j < MAX_FANG_PAIRS && j < ptr->fangs[i]; j++) {
			for (size_t k = MIN_FANG_PAIRS - 1; k < j; k++)
				fprintf(stdout, "\t");

			fprintf(stdout, "%llu %llu\n", ++local_count[j], ptr->number[i]);
		}
	}
	fflush(stdout);
}
#endif
