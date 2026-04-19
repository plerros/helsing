// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_CHECKPOINT_H
#define HELSING_CHECKPOINT_H // safety precaution for the c preprocessor

#include "other.h"

#include "taskboard.h"
#include "options.h"
#include "interval.h"

#if USE_CHECKPOINT
int touch_checkpoint(struct options_t options, struct interval_t interval);
int load_checkpoint(struct options_t options, struct interval_t *interval, struct taskboard *progress);
void save_checkpoint(struct options_t options, vamp_t complete, struct taskboard *progress);
#else /* USE_CHECKPOINT */
static inline int touch_checkpoint(
	ATTR_UNUSED struct options_t options,
	ATTR_UNUSED struct interval_t interval)
{
	return 0;
}
static inline int load_checkpoint(
	ATTR_UNUSED struct options_t options,
	ATTR_UNUSED struct interval_t *interval,
	ATTR_UNUSED struct taskboard *progress)
{
	return 0;
}
static inline void save_checkpoint(
	ATTR_UNUSED struct options_t options,
	ATTR_UNUSED vamp_t complete,
	ATTR_UNUSED struct taskboard *progress)
{
}
#endif /* USE_CHECKPOINT */
#endif /* HELSING_CHECKPOINT_H */
