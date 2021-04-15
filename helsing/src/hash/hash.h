/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_HASH_H
#define HELSING_HASH_H

#include "configuration.h"

#ifdef CHECKSUM_RESULTS
#include <openssl/evp.h>
#endif

#ifdef CHECKSUM_RESULTS
struct hash
{
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	unsigned char *md_value;
	int md_size;
};
void hash_new(struct hash **ptr);
void hash_free(struct hash *ptr);
void hash_print(struct hash *ptr);
#else /* CHECKSUM_RESULTS */
struct hash
{
};
static inline void hash_new(__attribute__((unused)) struct hash **ptr)
{
}
static inline void hash_free(__attribute__((unused)) struct hash *ptr)
{
}
static inline void hash_print(__attribute__((unused)) struct hash *ptr)
{
}
#endif /* CHECKSUM_RESULTS */
#endif /* HELSING_HASH_H */
