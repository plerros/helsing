// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_BTHANDLE_H
#define HELSING_BTHANDLE_H

#include "configuration.h"

#ifdef PROCESS_RESULTS

#include "btree.h"

struct bthandle
{
	struct btree *tree;
	vamp_t size;
};

struct bthandle *bthandle_init();
void bthandle_free(struct bthandle *handle);

void bthandle_add(struct bthandle *handle, vamp_t number);
void bthandle_reset(struct bthandle *handle);
void bthandle_cleanup(
	struct bthandle *handle,
	struct llhandle *lhandle,
	vamp_t number);

#else /* !PROCESS_RESULTS */

struct bthandle
{
};

static inline struct bthandle *bthandle_init()
{
	return NULL;
}

static inline void bthandle_free(
	__attribute__((unused)) struct bthandle *handle)
{
}

static inline void bthandle_add(
	__attribute__((unused)) struct bthandle *handle,
	__attribute__((unused)) vamp_t number)
{
}

static inline void bthandle_reset(
	__attribute__((unused)) struct bthandle *handle)
{
}

static inline void bthandle_cleanup(
	__attribute__((unused)) struct bthandle *handle,
	__attribute__((unused)) struct llhandle *lhandle,
	__attribute__((unused)) vamp_t number)
{
}

#endif /* PROCESS_RESULTS */

#endif /* HELSING_BTHANDLE_H */