// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <stdlib.h>
#include <assert.h>

#include "configuration.h"

#ifdef PROCESS_RESULTS

#include "llhandle.h"
#include "btree.h"

struct btree
{
	struct btree *left;
	struct btree *right;
	vamp_t value;
	length_t height; //Should probably be less than 32
	uint8_t fang_pairs;
};

void btree_init(struct btree **ptr, vamp_t value)
{
	struct btree *new = malloc(sizeof(struct btree));
	if (new == NULL)
		abort();

	new->left = NULL;
	new->right = NULL;
	new->height = 0;
	new->value = value;
	new->fang_pairs = 1;
	*ptr = new;
}

void btree_free(struct btree *tree)
{
	if (tree != NULL) {
		if (tree->left != NULL)
			btree_free(tree->left);
		if (tree->right != NULL)
			btree_free(tree->right);
	}
	free(tree);
}

int is_balanced(struct btree *tree)
{
	if (tree == NULL)
		return 0;

	int lheight = 0;
	int rheight = 0;

	if (tree->left != NULL)
		lheight = tree->left->height;
	if (tree->right != NULL)
		rheight = tree->right->height;

	return (lheight - rheight);
}

void btree_reset_height(struct btree *tree)
{
#if SANITY_CHECK
	assert(tree != NULL);
#endif

	tree->height = 0;
	if (tree->left != NULL && tree->left->height >= tree->height)
		tree->height = tree->left->height + 1;
	if (tree->right != NULL && tree->right->height >= tree->height)
		tree->height = tree->right->height + 1;

#if SANITY_CHECK
	assert(tree->height <= 32);
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

struct btree *btree_rotate_l(struct btree *tree)
{
	if (tree->right != NULL) {
		struct btree *right = tree->right;
		tree->right = right->left;
		btree_reset_height(tree);
		right->left = tree;
		btree_reset_height(right);
		return right;
	}
	return tree;
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

struct btree *btree_rotate_r(struct btree *tree)
{
	if (tree->left != NULL) {
		struct btree *left = tree->left;
		tree->left = left->right;
		btree_reset_height(tree);
		left->right = tree;
		btree_reset_height(left);
		return left;
	}
	return tree;
}

struct btree *btree_balance(struct btree *tree)
{
#if SANITY_CHECK
	assert(tree != NULL);
#endif

	int isbalanced = is_balanced(tree);
	if (isbalanced > 1) {
		if (is_balanced(tree->left) < 0) {
			tree->left = btree_rotate_l(tree->left);
			btree_reset_height(tree); //maybe optional?
		}
		tree = btree_rotate_r(tree);
	}
	else if (isbalanced < -1) {
		if (is_balanced(tree->right) > 0) {
			tree->right = btree_rotate_r(tree->right);
			btree_reset_height(tree); //maybe optional?
		}
		tree = btree_rotate_l(tree);

	}
	return tree;
}

struct btree *btree_add(
	struct btree *tree,
	vamp_t node,
	vamp_t *count)
{
	if (tree == NULL) {
		*count += 1;
		struct btree *tmpptr;
		btree_init(&tmpptr, node);
		return (tmpptr);
	}
	if (node == tree->value) {
		tree->fang_pairs += 1;
		return tree;
	}
	else if (node < tree->value)
		tree->left = btree_add(tree->left, node, count);
	else
		tree->right = btree_add(tree->right, node, count);

	btree_reset_height(tree);
	tree = btree_balance(tree);
	return tree;
}

struct btree *btree_cleanup(
	struct btree *tree,
	vamp_t number,
	struct llhandle *lhandle,
	vamp_t *btree_size)
{
	if (tree == NULL)
		return NULL;

	tree->right = btree_cleanup(tree->right, number, lhandle, btree_size);

	if (tree->value >= number) {
		if (tree->fang_pairs >= MIN_FANG_PAIRS)
			llhandle_add(lhandle, tree->value);
		struct btree *tmp = tree->left;
		tree->left = NULL;
		btree_free(tree);
		*btree_size -= 1;

		tree = btree_cleanup(tmp, number, lhandle, btree_size);
	}

	if (tree == NULL)
		return NULL;
	btree_reset_height(tree);
	tree = btree_balance(tree);

	return tree;
}

#endif /* PROCESS RESULTS */