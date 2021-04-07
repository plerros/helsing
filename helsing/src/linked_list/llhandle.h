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

#ifdef PROCESS_RESULTS
struct llhandle
{
	struct llist *head;
	vamp_t size;
};

#ifdef STORE_RESULTS
static inline void reset_head(struct llhandle *ptr)
{
	ptr->head = NULL;
}
#else /* STORE_RESULTS */
static inline void reset_head(__attribute__((unused)) struct llhandle *ptr)
{
}
#endif /* STORE_RESULTS */

void llhandle_init(struct llhandle **ptr);
void llhandle_free(struct llhandle *ptr);
void llhandle_add(struct llhandle *ptr, __attribute__((unused)) vamp_t value);
void llhandle_reset(struct llhandle *ptr);
#else /* PROCESS_RESULTS */
struct llhandle
{
};
static inline void llhandle_init(__attribute__((unused)) struct llhandle **ptr)
{
}
static inline void llhandle_free(__attribute__((unused)) struct llhandle *ptr)
{
}
static inline void llhandle_add(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) vamp_t value)
{
}
static inline void llhandle_reset(__attribute__((unused)) struct llhandle *ptr)
{
}
#endif /* PROCESS_RESULTS */

#if defined(STORE_RESULTS) && defined(PRINT_RESULTS)
void llhandle_print(struct llhandle *ptr, vamp_t count);
#else /* defined(STORE_RESULTS) && defined(PRINT_RESULTS)  */
static inline void llhandle_print(
	__attribute__((unused)) struct llhandle *ptr,
	__attribute__((unused)) vamp_t count)
{
}
#endif /* defined(STORE_RESULTS) && defined(PRINT_RESULTS)  */
#endif /* HELSING_LLHANDLE_H */