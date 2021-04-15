// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_BTHANDLE_H
#define HELSING_BTHANDLE_H

#include "configuration.h"
#include "llhandle.h"

#ifdef PROCESS_RESULTS
#include "btnode.h"
#endif

#ifdef PROCESS_RESULTS
struct bthandle
{
	struct btnode *node;
	vamp_t size;
};
void bthandle_new(struct bthandle **ptr);
void bthandle_free(struct bthandle *handle);
void bthandle_add(struct bthandle *handle, vamp_t key);
void bthandle_reset(struct bthandle *handle);
void bthandle_cleanup(
	struct bthandle *handle,
	struct llhandle *lhandle,
	vamp_t key);

#else /* !PROCESS_RESULTS */
struct bthandle
{
};
static inline void bthandle_new(__attribute__((unused)) struct bthandle **ptr)
{
}
static inline void bthandle_free(
	__attribute__((unused)) struct bthandle *handle)
{
}
static inline void bthandle_add(
	__attribute__((unused)) struct bthandle *handle,
	__attribute__((unused)) vamp_t key)
{
}
static inline void bthandle_reset(
	__attribute__((unused)) struct bthandle *handle)
{
}
static inline void bthandle_cleanup(
	__attribute__((unused)) struct bthandle *handle,
	__attribute__((unused)) struct llhandle *lhandle,
	__attribute__((unused)) vamp_t key)
{
}
#endif /* PROCESS_RESULTS */
#endif /* HELSING_BTHANDLE_H */