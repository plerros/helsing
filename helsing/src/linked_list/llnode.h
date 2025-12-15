// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025 Pierro Zachareas
 */

#ifndef HELSING_LLNODE_H
#define HELSING_LLNODE_H

#include "configuration_adv.h"

struct llvamp_t;
void llvamp_new(struct llvamp_t **ptr, struct llvamp_t *next);
void llvamp_free(struct llvamp_t *node);
void llvamp_add(struct llvamp_t **ptr, vamp_t value);
struct llvamp_t *llvamp_pop(struct llvamp_t **ptr);
vamp_t *llvamp_getdata(struct llvamp_t *ptr);
size_t llvamp_count_elements(struct llvamp_t *ptr);
#endif /* HELSING_LLNODE_H */
