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

struct tile *tile_init(vamp_t lmin, vamp_t lmax);
void tile_free(struct tile *ptr);
#endif /* HELSING_TILE_H */