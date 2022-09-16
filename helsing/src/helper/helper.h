// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_HELPER_H
#define HELSING_HELPER_H

#include <stdbool.h>

#include "configuration_adv.h"

void no_args();
bool willoverflow(vamp_t x, vamp_t limit, digit_t digit);
length_t length(vamp_t x);
vamp_t pow_v(length_t exponent);
vamp_t get_min(vamp_t min, vamp_t max);
vamp_t get_max(vamp_t min, vamp_t max);
vamp_t div_roof(vamp_t x, vamp_t y);
length_t partition3(length_t x);

#endif /* HELPER_HELSING */
