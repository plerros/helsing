// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026 Pierro Zachareas
 */

#ifndef HELSING_NUMERAL_BASE_H
#define HELSING_NUMERAL_BASE_H

/*
 * BASE:
 *
 * 	BASE defines the base of the numeral system to be used by the vampire
 * checking algorithm.
 *
 * For bases above 255 adjust digit_t accordingly.
 * If 2^(ELEMENT_BITS/(BASE-1)) < ELEMENT_BITS/log2(BASE-1), then disable ALG_CACHE.
 */

#define BASE 10
	#if (BASE < 2)
		#error BASE must be larger than 1
	#endif

#endif /* HELSING_NUMERAL_BASE_H */