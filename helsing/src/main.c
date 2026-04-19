// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2024 Pierro Zachareas
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <threads.h>

#include "datatypes.h"

#include "helper.h"
#include "taskboard.h"
#include "targs_handle.h"
#include "checkpoint.h"
#include "interval.h"
#include "options.h"

static vamp_t get_lmax(vamp_t lmin, vamp_t max)
{
	if (length(lmin) < length(VAMP_MAX())) {
		vamp_t lmax = pow_v(length(lmin)) - 1;
		if (lmax < max)
			return lmax;
	}
	return max;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	struct options_t *options = NULL;
	struct interval_t interval;
	struct taskboard *progress = NULL;

	rc = options_new(&options, argc, argv);
	if (rc)
		goto out;
	rc = interval_set(&interval, *options);
	if (rc)
		goto out;
	if (options_touch_checkpoint(*options))
		rc = touch_checkpoint(*options, interval);
	if (rc)
		goto out;

	taskboard_new(&progress, *options);

	if (load_checkpoint(*options, &interval, progress))
		goto out;

	thrd_t *threads = malloc(sizeof(thrd_t) * options->threads);
	if (threads == NULL)
		abort();
	struct targs_handle *thhandle = NULL;
	targs_handle_new(&thhandle, *options, interval.min, interval.max, progress);

	vamp_t lmin = 0, lmax = 0;
	for (; interval.complete < interval.max; interval.complete = lmax) {
		lmin = get_min(interval.complete + 1,  interval.max);
		lmax = get_lmax(lmin, interval.max);
		taskboard_set(progress, lmin, lmax);
		if (progress->size == 0)
			continue;

		helsing_fprint(stderr, "svsvs",  "Checking interval: [", lmin, ", ", lmax, "]\n");
		for (thread_t thread = 0; thread < options->threads; thread++)
			assert(thrd_create(&threads[thread], thread_function, (void *)(thhandle->targs[thread])) == thrd_success);
		for (thread_t thread = 0; thread < options->threads; thread++)
			thrd_join(threads[thread], 0);
	}
	targs_handle_print(thhandle);
	targs_handle_free(thhandle);
	free(threads);
out:
	taskboard_free(progress);
	options_free(options);
	return rc;
}
