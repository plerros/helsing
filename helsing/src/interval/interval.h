// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_INTERVAL_H
#define HELSING_INTERVAL_H

#include "datatypes.h"

#include "options.h"

struct interval_t
{
	vamp_t min;
	vamp_t max;
	vamp_t complete;
};

int interval_set(struct interval_t *ptr, struct options_t options);
int interval_set_complete(struct interval_t *ptr, vamp_t complete);

#endif /* HELSING_INTERVAL_H */
