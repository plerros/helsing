// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef HELSING_LLHANDLE_H
#define HELSING_LLHANDLE_H

#include "configuration.h"

#ifdef STORE_RESULTS
#include "llist.h"
#endif


struct llhandle
{
#ifdef STORE_RESULTS
	struct llist *head;
#endif
	vamp_t size;
};

#ifdef STORE_RESULTS

static inline void reset_head(struct llhandle *ptr)
{
	ptr->head = NULL;
}

static inline void free_head(struct llhandle *ptr)
{
	llist_free(ptr->head);
	ptr->head = NULL;
}

static inline void init_head(struct llhandle *ptr,vamp_t value)
{
	ptr->head = llist_init(value, ptr->head);
}

#else /* STORE_RESULTS */

static inline void reset_head(__attribute__((unused)) struct llhandle *ptr)
{
}

static inline void free_head(__attribute__((unused)) struct llhandle *ptr)
{
}

static inline void init_head(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) vamp_t value)
{
}

#endif /* STORE_RESULTS */

struct llhandle *llhandle_init();
void llhandle_free(struct llhandle *handle);
void llhandle_add(struct llhandle *handle, __attribute__((unused)) vamp_t value);
void llhandle_reset(struct llhandle *handle);

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS) 

void llhandle_print(struct llhandle *ptr, vamp_t count);

#else /* defined(STORE_RESULTS) && defined(PRINT_RESULTS)  */

static inline void llhandle_print(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) vamp_t count)
{
}

#endif /* defined(STORE_RESULTS) && defined(PRINT_RESULTS)  */

#endif