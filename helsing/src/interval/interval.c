// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2022 Pierro Zachareas
 */

#include <stdio.h>

#include "configuration_adv.h"
#include "helper.h"
#include "interval.h"
#include "options.h"
#include "cache.h"

int interval_set(struct interval_t *ptr, struct options_t options)
{
	int rc = 0;
	if (options.min > options.max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		fprintf(stderr, "Invalid arguments, %ju <= %ju\n", (uintmax_t)(options.min), (uintmax_t)(options.max));
		rc = 1;
		goto out;
	}

	ptr->min = get_min(options.min, options.max);
	if (options.min != ptr->min)
		fprintf(stderr, "Adjusted min from %ju to %ju\n", (uintmax_t)(options.min), (uintmax_t)(ptr->min));

	ptr->max = get_max(ptr->min, options.max);
	if (options.max != ptr->max)
		fprintf(stderr, "Adjusted max from %ju to %ju\n", (uintmax_t)(options.max), (uintmax_t)(ptr->max));

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
