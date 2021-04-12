// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <openssl/evp.h>

#include "configuration.h"
#include "llnode.h"
#include "llhandle.h"

#ifdef PROCESS_RESULTS
void llhandle_init(struct llhandle **ptr)
{
	struct llhandle *new = malloc(sizeof(struct llhandle));
	if (new == NULL)
		abort();

	new->first = NULL;
	new->size = 0;
	*ptr = new;
}

void llhandle_free(struct llhandle *ptr)
{
	llnode_free(ptr->first);
	free(ptr);
}

void llhandle_add(struct llhandle *ptr, vamp_t value)
{
	if (ptr == NULL)
		return;

	llnode_init(&(ptr->first), value, ptr->first);
	ptr->size += 1;
}

void llhandle_reset(struct llhandle *ptr)
{
	llnode_free(ptr->first);
	ptr->first = NULL;
	ptr->size = 0;
}

#endif /* PROCESS_RESULTS */

#if defined(PROCESS_RESULTS) && defined(CHECKSUM_RESULTS)
void llhandle_checksum(struct llhandle *ptr, EVP_MD_CTX *mdctx, EVP_MD *md, unsigned char *md_value)
{
	llnode_checksum(ptr->first, mdctx, md, md_value);
}
#endif

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void llhandle_print(struct llhandle *ptr, vamp_t count)
{
	llnode_print(ptr->first, count);
}
#endif