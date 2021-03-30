// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_CHECKPOINT_H
#define HELSING_CHECKPOINT_H // safety precaution for the c preprocessor

#include "configuration.h"

#if defined(USE_CHECKPOINT) && USE_CHECKPOINT
void touch_checkpoint(vamp_t min, vamp_t max);
void load_checkpoint(vamp_t *min, vamp_t *max, vamp_t *current, vamp_t *count);
void save_checkpoint(vamp_t current, vamp_t count);
#else /* defined(USE_CHECKPOINT) && USE_CHECKPOINT */
static inline void touch_checkpoint(
	__attribute__((unused)) vamp_t min,
	__attribute__((unused)) vamp_t max)
{
}
static inline void load_checkpoint(
	__attribute__((unused)) vamp_t *min,
	__attribute__((unused)) vamp_t *max,
	__attribute__((unused)) vamp_t *current,
	__attribute__((unused)) vamp_t *count)
{
}
static inline void save_checkpoint(
	__attribute__((unused)) vamp_t current,
	__attribute__((unused)) vamp_t count)
{
}
#endif /* defined(USE_CHECKPOINT) && USE_CHECKPOINT */
#endif /* HELSING_CHECKPOINT_H */