// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include "configuration.h"
#include "configuration_adv.h"

#ifdef PROCESS_RESULTS
#include <stdlib.h>
#include "llhandle.h"
#include "btnode.h"
#endif

#if defined (PROCESS_RESULTS) && SANITY_CHECK
#include <assert.h>
#endif

#ifdef PROCESS_RESULTS

void btnode_new(struct btnode **ptr, vamp_t key)
{
	if (ptr == NULL)
		return;

	struct btnode *new = malloc(sizeof(struct btnode));
	if (new == NULL)
		abort();

	new->left = NULL;
	new->right = NULL;
	new->height = 0;
	new->key = key;
	new->fang_pairs = 1;
	*ptr = new;
}

void btnode_free(struct btnode *node)
{
	if (node == NULL)
		return;

	if (node->left != NULL)
		btnode_free(node->left);
	if (node->right != NULL)
		btnode_free(node->right);
	free(node);
}

static int is_balanced(struct btnode *node)
{
	if (node == NULL)
		return 0;

	int lheight = 0;
	int rheight = 0;

	if (node->left != NULL)
		lheight = node->left->height;
	if (node->right != NULL)
		rheight = node->right->height;

	return (lheight - rheight);
}

static void btnode_reset_height(struct btnode *node)
{
#if SANITY_CHECK
	assert(node != NULL);
#endif

	node->height = 0;
	if (node->left != NULL && node->left->height >= node->height)
		node->height = node->left->height + 1;
	if (node->right != NULL && node->right->height >= node->height)
		node->height = node->right->height + 1;

#if SANITY_CHECK
	assert(node->height <= 32);
#endif
}

/*
 * Binary tree left rotation:
 *
 *     A                 B
 *    / \               / \
 *  ...  B     -->     A  ...
 *      / \           / \
 *     C  ...       ...  C
 *
 * The '...' are completely unaffected.
 */

static struct btnode *btnode_rotate_l(struct btnode *node)
{
	if (node->right != NULL) {
		struct btnode *right = node->right;
		node->right = right->left;
		btnode_reset_height(node);
		right->left = node;
		btnode_reset_height(right);
		return right;
	}
	return node;
}

/*
 * Binary tree right rotation:
 *
 *       A             B
 *      / \           / \
 *     B  ...  -->  ...  A
 *    / \               / \
 *  ...  C             C  ...
 *
 * The '...' are completely unaffected.
 */

static struct btnode *btnode_rotate_r(struct btnode *node)
{
	if (node->left != NULL) {
		struct btnode *left = node->left;
		node->left = left->right;
		btnode_reset_height(node);
		left->right = node;
		btnode_reset_height(left);
		return left;
	}
	return node;
}

static struct btnode *btnode_balance(struct btnode *node)
{
#if SANITY_CHECK
	assert(node != NULL);
#endif

	int isbalanced = is_balanced(node);
	if (isbalanced > 1) {
		if (is_balanced(node->left) < 0) {
			node->left = btnode_rotate_l(node->left);
			btnode_reset_height(node); //maybe optional?
		}
		node = btnode_rotate_r(node);
	}
	else if (isbalanced < -1) {
		if (is_balanced(node->right) > 0) {
			node->right = btnode_rotate_r(node->right);
			btnode_reset_height(node); //maybe optional?
		}
		node = btnode_rotate_l(node);

	}
	return node;
}

struct btnode *btnode_add(
	struct btnode *node,
	vamp_t key,
	vamp_t *count)
{
	if (node == NULL) {
		*count += 1;
		struct btnode *tmpptr;
		btnode_new(&tmpptr, key);
		return (tmpptr);
	}
	if (key == node->key) {
		node->fang_pairs += 1;
		return node;
	}
	else if (key < node->key)
		node->left = btnode_add(node->left, key, count);
	else
		node->right = btnode_add(node->right, key, count);

	btnode_reset_height(node);
	node = btnode_balance(node);
	return node;
}

struct btnode *btnode_cleanup(
	struct btnode *node,
	vamp_t key,
	struct llhandle *lhandle,
	vamp_t *btnode_size)
{
	if (node == NULL)
		return NULL;

	node->right = btnode_cleanup(node->right, key, lhandle, btnode_size);

	if (node->key >= key) {
		if (node->fang_pairs >= MIN_FANG_PAIRS)
			llhandle_add(lhandle, node->key);
		struct btnode *tmp = node->left;
		node->left = NULL;
		btnode_free(node);
		*btnode_size -= 1;

		node = btnode_cleanup(tmp, key, lhandle, btnode_size);
	}

	if (node == NULL)
		return NULL;
	btnode_reset_height(node);
	node = btnode_balance(node);

	return node;
}

#endif /* PROCESS RESULTS */