// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdio.h>
#include <openssl/evp.h>

#include "configuration.h"

#ifdef STORE_RESULTS

#include "llnode.h"

void llnode_init(struct llnode **ptr, vamp_t value , struct llnode *next)
{
	struct llnode *new;
	if (next != NULL && next->current < LINK_SIZE) {
		new = next;
		new->value[new->current] = value;
		new->current += 1;
	} else {
		new = malloc(sizeof(struct llnode));
		if (new == NULL)
			abort();

		new->value[0] = value;
		new->current = 1;
		new->next = next;
	}
	*ptr = new;
}

void llnode_free(struct llnode *node)
{
	if (node != NULL) {
		struct llnode *tmp = node;
		for (struct llnode *i = tmp; tmp != NULL; i = tmp) {
			tmp = tmp->next;
			free(i);
		}
	}
}

#endif /* STORE_RESULTS */

#if defined(STORE_RESULTS) && defined(CHECKSUM_RESULTS)
void llnode_checksum(struct llnode *node, EVP_MD_CTX *mdctx)
{
	for (struct llnode *i = node; i != NULL ; i = i->next) {
		for (uint16_t j = i->current; j > 0 ; j--) {
			vamp_t tmp = i->value[j - 1];

			#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
			tmp = __builtin_bswap64(tmp);
			#endif

			EVP_DigestUpdate(mdctx, &tmp, sizeof(tmp));
		}
	}
}
#endif

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
void llnode_print(struct llnode *node, vamp_t count)
{
	for (struct llnode *i = node; i != NULL ; i = i->next) {
		for (uint16_t j = i->current; j > 0 ; j--) {
			fprintf(stdout, "%llu %llu\n", ++count, i->value[j - 1]);
			fflush(stdout);
		}
	}
}
#endif