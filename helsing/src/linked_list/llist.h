// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_LLIST_H
#define HELSING_LLIST_H

#include <stdio.h>

#include "configuration.h"

#ifdef STORE_RESULTS

struct llist
{
	vamp_t value[LINK_SIZE];
	uint16_t current;
	struct llist *next;
};

struct llist *llist_init(vamp_t value , struct llist *next);
void llist_free(struct llist *list);

#else /* STORE_RESULTS */

struct llist
{
};

static inline struct llist *llist_init(
	__attribute__((unused)) vamp_t value,
	__attribute__((unused)) struct llist *next)
{
	return NULL;
}

static inline void llist_free(
	__attribute__((unused)) struct llist *list)
{
}

#endif /* STORE_RESULTS */

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)

void llist_print(struct llist *list, vamp_t count);

#else /* defined(STORE_RESULTS) && defined(PRINT_RESULTS) */

static inline void llist_print(
	__attribute__((unused)) struct llist *list,
	__attribute__((unused)) vamp_t count)
{
}

#endif /* defined(STORE_RESULTS) && defined(PRINT_RESULTS) */

#endif