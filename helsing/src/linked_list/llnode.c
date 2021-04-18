// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#ifdef STORE_RESULTS
#include <stdlib.h>
#include <stdio.h>
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

void llnode_new(struct llnode **ptr, vamp_t value, struct llnode *next)
{
	if (ptr == NULL)
		return;

	struct llnode *new = malloc(sizeof(struct llnode));
	if (new == NULL)
		abort();

	new->value[0] = value;
	new->current = 1;
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
		free(i);
	}
}

void llnode_add(struct llnode **ptr, vamp_t value, struct llnode *next)
{
#if SANITY_CHECK
	assert(ptr != NULL);
#endif
	struct llnode *new;
	if (next != NULL && next->current < LINK_SIZE) {
		new = next;
		new->value[new->current] = value;
		new->current += 1;
	} else {
		llnode_new(&new, value, next);
	}
	*ptr = new;
}

#endif /* STORE_RESULTS */

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
void llnode_checksum(struct llnode *node, struct hash *checksum)
{
	for (struct llnode *i = node; i != NULL; i = i->next) {
		for (uint16_t j = i->current; j > 0; j--) {
			vamp_t tmp = i->value[j - 1];

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
		for (uint16_t j = i->current; j > 0; j--) {
			fprintf(stdout, "%llu %llu\n", ++count, i->value[j - 1]);
			fflush(stdout);
		}
	}
}
#endif