// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdio.h>

#include "configuration_adv.h"
#include "helper.h"
#include "interval.h"
#include "cache.h"

int interval_set(struct interval_t *ptr, vamp_t min, vamp_t max)
{
	int rc = 0;
	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		rc = 1;
		goto out;
	}

	ptr->min = get_min(min, max);
	if (min != ptr->min)
		fprintf(stderr, "Adjusted min from %llu to %llu\n", min, ptr->min);

	ptr->max = get_max(ptr->min, max);
	if (max != ptr->max)
		fprintf(stderr, "Adjusted max from %llu to %llu\n", max, ptr->max);

	if (cache_ovf_chk(ptr->max)) {
		fprintf(stderr, "WARNING: the code might produce false positives, ");
		if (ELEMENT_BITS == 32)
			fprintf(stderr, "please set ELEMENT_BITS to 64.\n");
		else
			fprintf(stderr, "please set CACHE to false.\n");
		rc = 1;
		goto out;
	}

	ptr->complete = 0;
	if (ptr->complete < ptr->min)
		ptr->complete = ptr->min - 1;

	/*
	 * The following handles situations like [BASE^2, BASE^3 -1], where
	 * there can be no vampire numbers within the interval.
	 */
	if (length(ptr->min) % 2 == 1)
		ptr->complete = ptr->min;

out:
	return rc;
}

int interval_set_complete(struct interval_t *ptr, vamp_t complete)
{
	if (complete < ptr->min) {
		if (get_min(complete + 1, ptr->max) < ptr->min)
			return 1;
	}
	else if (complete > ptr->max) {
		return 1;
	}
	else if (complete < ptr->complete) {
		return 1;
	}

	ptr->complete = complete;
	return 0;
}
