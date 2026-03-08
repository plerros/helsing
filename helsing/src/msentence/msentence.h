// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025-2026 Pierro Zachareas
 */

#ifndef HELSING_MSENTENCE_H
#define HELSING_MSENTENCE_H

/*
 * msentence_t
 *
 * multiplication sentence (multiplier x multiplicand = product)
 */

struct msentence_t {
	fang_t multiplier;
	fang_t multiplicand;
	vamp_t product;
};

#endif /* HELSING_MSENTENCE_H */
