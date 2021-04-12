// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_LLHANDLE_H
#define HELSING_LLHANDLE_H

#include <openssl/evp.h>

#include "configuration.h"
#include "llnode.h"

#ifdef PROCESS_RESULTS
struct llhandle
{
	struct llnode *first;
	vamp_t size;
};
void llhandle_init(struct llhandle **ptr);
void llhandle_free(struct llhandle *ptr);
void llhandle_add(struct llhandle *ptr, vamp_t value);
void llhandle_reset(struct llhandle *ptr);
#else /* PROCESS_RESULTS */
struct llhandle
{
};
static inline void llhandle_init(__attribute__((unused)) struct llhandle **ptr)
{
	struct llhandle *new = malloc(sizeof(struct llhandle));
	if (new == NULL)
		abort();
	*ptr = new;
}
static inline void llhandle_free(__attribute__((unused)) struct llhandle *ptr)
{
	free(ptr);
}
static inline void llhandle_add(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) vamp_t value)
{
}
static inline void llhandle_reset(__attribute__((unused)) struct llhandle *ptr)
{
}
#endif /* PROCESS_RESULTS */

#if defined(PROCESS_RESULTS) && defined(CHECKSUM_RESULTS)
void llhandle_checksum(struct llhandle *ptr, EVP_MD_CTX *mdctx, EVP_MD *md, unsigned char *md_value);
#else
static inline void llhandle_checksum(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) EVP_MD_CTX *mdctx,
	__attribute__((unused)) EVP_MD *md,
	__attribute__((unused)) unsigned char *md_value)
{
}
#endif

#if defined(PROCESS_RESULTS) && defined(PRINT_RESULTS)
void llhandle_print(struct llhandle *ptr, vamp_t count);
#else
static inline void llhandle_print(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) vamp_t count)
{
}
#endif
#endif /* HELSING_LLHANDLE_H */