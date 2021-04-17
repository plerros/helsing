// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_CHECKPOINT_H
#define HELSING_CHECKPOINT_H // safety precaution for the c preprocessor

#include "configuration.h"
#include "configuration_adv.h"
#include "taskboard.h"

#if USE_CHECKPOINT
int touch_checkpoint(vamp_t min, vamp_t max);
int load_checkpoint(
	vamp_t *min,
	vamp_t *max,
	vamp_t *current,
	struct taskboard *progress);
void save_checkpoint(vamp_t current, struct taskboard *progress);
#else /* USE_CHECKPOINT */
static inline int touch_checkpoint(
	__attribute__((unused)) vamp_t min,
	__attribute__((unused)) vamp_t max)
{
	return 0;
}
static inline int load_checkpoint(
	__attribute__((unused)) vamp_t *min,
	__attribute__((unused)) vamp_t *max,
	__attribute__((unused)) vamp_t *current,
	__attribute__((unused)) struct taskboard *progress)
{
	return 0;
}
static inline void save_checkpoint(
	__attribute__((unused)) vamp_t current,
	__attribute__((unused)) struct taskboard *progress)
{
}
#endif /* USE_CHECKPOINT */
#endif /* HELSING_CHECKPOINT_H */