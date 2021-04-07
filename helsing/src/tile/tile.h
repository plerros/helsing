// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_TILE_H
#define HELSING_TILE_H

#include "configuration.h"

#ifdef PROCESS_RESULTS
#include "llhandle.h"
#endif

struct tile
{
	vamp_t lmin; // local minimum
	vamp_t lmax; // local maximum
	struct llhandle *result;
};

struct tile *tile_init(vamp_t min, vamp_t max);
void tile_free(struct tile *ptr);

#ifdef PROCESS_RESULTS
static inline void tile_init_result(struct tile *ptr)
{
	ptr->result = NULL;
}
#else /* PROCESS_RESULTS */
static inline void tile_init_result(__attribute__((unused)) struct tile *ptr)
{
}
#endif /* PROCESS_RESULTS */
#endif /* HELSING_TILE_H */