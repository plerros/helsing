/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_HASH_H
#define HELSING_HASH_H

#if (VAMPIRE_NUMBER_OUTPUTS) && (VAMPIRE_HASH)
#include <stdint.h>
#include <openssl/evp.h>
struct hash
{
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	uint8_t *md_value;
	int md_size;
};
void hash_new(struct hash **ptr);
void hash_free(struct hash *ptr);
void hash_print(struct hash *ptr);
#else /* VAMPIRE_NUMBER_OUTPUTS && VAMPIRE_HASH */
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
#endif /* VAMPIRE_NUMBER_OUTPUTS && VAMPIRE_HASH */
#endif /* HELSING_HASH_H */
