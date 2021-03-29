// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <stdio.h>

#include "configuration.h"

#ifdef STORE_RESULTS

#include "llist.h"

struct llist *llist_init(vamp_t value , struct llist *next)
{
	struct llist *new;
	if (next != NULL && next->current < LINK_SIZE) {
		new = next;
		new->value[new->current] = value;
		new->current += 1;
	} else {
		new = malloc(sizeof(struct llist));
		if (new == NULL)
			abort();

		new->value[0] = value;
		new->current = 1;
		new->next = next;
	}
	return new;
}

void llist_free(struct llist *list)
{
	if (list != NULL) {
		struct llist *tmp = list;
		for (struct llist *i = tmp; tmp != NULL; i = tmp) {
			tmp = tmp->next;
			free(i);
		}
	}
}

#endif /* STORE_RESULTS */

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)

void llist_print(struct llist *list, vamp_t count)
{
	for (struct llist *i = list; i != NULL ; i = i->next) {
		for (uint16_t j = i->current; j > 0 ; j--) {
			fprintf(stdout, "%llu %llu\n", ++count, i->value[j - 1]);
			fflush(stdout);
		}
	}
}

#endif /* defined(STORE_RESULTS) && defined(PRINT_RESULTS) */