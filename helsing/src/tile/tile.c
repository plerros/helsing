// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdbool.h>

#include "configuration.h"
#include "tile.h"

struct tile *tile_init(vamp_t min, vamp_t max)
{
	struct tile *new = malloc(sizeof(struct tile));
	if (new == NULL)
		abort();

	new->lmin = min;
	new->lmax = max;
	tile_init_result(new);
	tile_init_complete(new);
	return new;
}

void tile_free(struct tile *ptr)
{
	tile_free_result(ptr);
	free(ptr);
}