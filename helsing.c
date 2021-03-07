/*
 * Copyright (c) 2021, Pierro Zachareas, et al.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>	// pow
#include <ctype.h>	// isdigit
#include <string.h>

// Compile with: gcc -O3 -Wall -Wextra -pthread -lm -o helsing helsing.c
// Check memory with valgrind --tool=massif
// Check threads with thread sanitizer -fsanitize=thread

/*--------------------------- COMPILATION OPTIONS ---------------------------*/
#define THREADS 1
#define thread_t uint16_t

#define MEASURE_RUNTIME false
#define SANITY_CHECK false

/*
 * DIST_COMPENSATION:
 *
 * 	Based on results, I produced 6 functions that try to estimate the
 * distribution of vampire numbers. The integral of the inverse can help with
 * load distribution between threads, minimizing the need for load balancing
 * and its overhead. In practice I haven't noticed any performance increase.
 *
 * 	You can set DIST_COMPENSATION to false to disable it, or any value
 * between 1 and 6 to select a specific version.
 */

#define DIST_COMPENSATION false
#define PRINT_DIST_MATRIX false

/*
 * TILE_SIZE:
 *
 * Maximum value: 18446744073709551615ULL (2^64 -1)
 *
 * 	Threads will pick and complete available tiles and then optionally
 * store the results. Later, the results get processed, optionally printed and
 * freed in order, so that they don't get mixed up.
 *
 * TILE_SIZE should be set as unsigned long long (ULL extension at the end).
 *
 * 	You can manually optimize the code for specific [min,max] runs, to use
 * less memory by setting AUTO_TILE_SIZE to false and adjusting TILE_SIZE.
 */

#define AUTO_TILE_SIZE true
#define TILE_SIZE 18446744073709551615ULL

/*
 * JENS_K_A_OPTIMIZATION:
 *
 * 	This code was originally written by Jens Kruse Andersen, and is
 * included with their permission. Adjustments were made to reduce 
 * computational complexity and memory usage, to improve memory access patterns
 * and to accomodate features such as multithreading.
 *
 * Source: http://primerecords.dk/vampires/index.htm
 *
 * There are two things that this optimization does:
 *
 * 1) A trick that can reduce computational complexity. Here is an example
 * 	that illustrates that:
 *
 * 	Given a set of numbers {123456, 125634, 345612}, in order to convert
 * 	them to arrays of digits, 3 * 6 = 18 modulo and division operations are
 * 	required. However if we calculate and store the {12, 34, 56}, we can
 * 	reconstruct the original numbers {[12][34][56], [12][56][34],
 * 	[34][56][12]} and their arrays of digits, with only 3 * 2 = 6 modulo
 *	and division operations.
 *
 * 2) Minimize memory usage by storing less elements of smaller size.
 *
 * 	Given a product A and its fangs B & C, in order to store the sum of
 *	each digit, we can use an array of 10 elements.
 *
 *	Because B & C are both fangs it will be always true that arr_A and
 *	arr_B + arr_C have the same total sum of digits and therefore we can
 *	avoid storing one of the elements. I chose not to store the 0s.
 *
 *	The minimum size of each element is roof(log2(roof(log10(2 ^ n)))),
 * 	where n are the bits used to represent the number A. For a 64-bit
 *	unsigned integer that would be 5 bits.
 *
 *	To avoid memory alignment issues, we are going to use a single 32 or 64
 *	bit unsigned integer, which we are going to treat as the aforementioned
 *	array with some adjustments to minimize padding overhead.
 *
 *	Last but not least I think in some cases it might be possible to get
 *	away with not storing 1s too. So far I haven't noticed any false
 *	positives for values below 10^16, but because there is no proof,
 *	DIG_ELEMENT_BITS is set by default to 64.
 *
 * 	For example a 32-bit sized element gets split into 8 * 4-bit segments
 * 	and a 64-bit sized element gets split into 9 * 7-bit segments:
 * 	                                99998888777766665555444433332222
 * 	X999999988888887777777666666655555554444444333333322222221111111
 * 	|       |       |       |       |       |       |       |      |
 * 	63      55      47      39      31      23      15      7      0
 */

#define JENS_K_A_OPTIMIZATION true
#define DIG_ELEMENT_BITS 64

/*
 * VERBOSE_LEVEL:
 *
 * 0 - Count fang pairs
 * 1 - Print fang pairs
 * 2 - Count vampire numbers
 * 3 - OEIS
 */

#define VERBOSE_LEVEL 0

/*
 * Both vamp_t and fang_t must be unsigned, vamp_t should be double the size
 * of fang_t and fang_max, vamp_max should be set accordingly.
 *
 * You should be able to change vamp_t up to 256-bit without any issues.
 * If you want to go any higher check the uint8_t for overflow.
*/

typedef unsigned long long vamp_t;
#define vamp_max ULLONG_MAX

typedef unsigned long fang_t;
#define fang_max ULONG_MAX

typedef uint8_t digit_t;
typedef uint8_t length_t;

/*--------------------------- PREPROCESSOR STUFF  ---------------------------*/
//DIGMULT = DIG_ELEMENT_BITS/(10 - DIGSKIP)
#if DIG_ELEMENT_BITS == 32
	typedef uint32_t digits_t;
	#define DIGSKIP 2
	#define DIGMULT 4
#elif DIG_ELEMENT_BITS == 64
	typedef uint64_t digits_t;
	#define DIGSKIP 1
	#define DIGMULT 7
#endif

#if MEASURE_RUNTIME
	#if defined(CLOCK_MONOTONIC)
		#define SPDT_CLK_MODE CLOCK_MONOTONIC
	#elif defined(CLOCK_REALTIME)
		#define SPDT_CLK_MODE CLOCK_REALTIME
	#endif
#endif

#if (VERBOSE_LEVEL == 0)
	#define COUNT_RESULTS
#elif (VERBOSE_LEVEL == 1)
	#define DUMP_RESULTS
#elif (VERBOSE_LEVEL == 2)
	#define PROCESS_RESULTS
#elif (VERBOSE_LEVEL == 3)
	#define STORE_RESULTS
	#define PROCESS_RESULTS
	#define PRINT_RESULTS
#endif

/*---------------------------- HELPER FUNCTIONS  ----------------------------*/

void print_licence()
{
	printf("\n\
Copyright (c) 2021, Pierro Zachareas, et al.\n\
All rights reserved.\n\
\n\
Redistribution and use in source and binary forms, with or without\n\
modification, are permitted provided that the following conditions are met:\n\
1. Redistributions of source code must retain the above copyright notice, this\n\
   list of conditions and the following disclaimer.\n\
\n\
2. Redistributions in binary form must reproduce the above copyright notice,\n\
   this list of conditions and the following disclaimer in the documentation\n\
   and/or other materials provided with the distribution.\n\
\n\
3. Neither the name of the copyright holder nor the names of its contributors\n\
   may be used to endorse or promote products derived from this software\n\
   without specific prior written permission.\n\
\n\
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n\
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n\
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n\
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n\
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n\
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n\
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n\
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n\
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  OF THE USE\n\
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
	\n");
}

length_t length(vamp_t x) // bugfree
{
	length_t length = 0;
	for (; x > 0; x /= 10)
		length++;
	return length;
}

bool length_isodd(vamp_t x) // bugfree
{
	return (length(x) % 2);
}

void sanitycheck([[maybe_unused]] bool exp)
{
#if SANITY_CHECK
	assert(exp);
#endif
}

// pow10 for vampire type.
vamp_t pow10v(length_t exponent) // bugfree
{
	sanitycheck(exponent <= length(vamp_max) - 1);
	vamp_t base = 1;
	for (; exponent > 0; exponent--)
		base *= 10;
	return base;
}

// willoverflow: Checks if (10 * x + digit) will overflow.
bool willoverflow(vamp_t x, digit_t digit) // bugfree
{
	sanitycheck(digit < 10);
	if (x > vamp_max / 10)
		return true;
	if (x == vamp_max / 10 && digit > vamp_max % 10)
		return true;
	return false;
}

// ASCII to vampire type
vamp_t atov(const char *str, bool *err) // bugfree
{
	sanitycheck(str != NULL);
	sanitycheck(err != NULL);
	vamp_t number = 0;
	for (length_t i = 0; isdigit(str[i]); i++) {
		digit_t digit = str[i] - '0';
		if (willoverflow(number, digit)) {
			*err = true;
			return 1;
		}
		number = 10 * number + digit;
	}
	return number;
}

bool notrailingzero(fang_t x) // bugfree
{
	return ((x % 10) != 0);
}

vamp_t get_min(vamp_t min, vamp_t max)
{
	if (length_isodd(min)) {
		length_t min_length = length(min);
		if (min_length < length(max))
			min = pow10v(min_length);
		else
			min = max;
	}
	return min;
}

vamp_t get_max(vamp_t min, vamp_t max)
{
	if (length_isodd(max)) {
		length_t max_length = length(max);
		if (max_length > length(min))
			max = pow10v(max_length - 1) - 1;
		else
			max = min;
	}
	return max;
}

vamp_t get_lmax(vamp_t lmin, vamp_t max)
{
	vamp_t lmax;
	if (length(lmin) < length(vamp_max)) {
		lmax = pow10v(length(lmin)) - 1;
		if (lmax < max)
			return lmax;
	}
	return max;
}

// Vampire square root to fang.
fang_t sqrtv_floor(vamp_t x)
{
	vamp_t x2 = x / 2;
	vamp_t root = x2; // Initial estimate
	if (root > 0) { // Sanity check
		vamp_t tmp = (root  + x / root) / 2; // Update
		while (tmp < root) { // This also checks for cycle
			root = tmp;
			tmp = (root  + x / root) / 2;
		}
		return root;
	}
	return x;
}

fang_t sqrtv_roof(vamp_t x)
{
	fang_t root = sqrtv_floor(x);
	if (root == fang_max)
		return root;

	return (x / root);
}

// Modulo 9 lack of congruence
bool con9(vamp_t x, vamp_t y)
{
	return ((x + y) % 9 != (x * y) % 9);
}

vamp_t div_roof (vamp_t x, vamp_t y)
{
	return (x/y + !!(x%y));
}

#if DIST_COMPENSATION
long double distribution_inverted_integral(long double area)
{
	long double exponent;
	#if (DIST_COMPENSATION == 1)
		exponent = 1.0/(3.0-(0.7*area));
		return (1.0 - 0.9 * powl(1.0-area, exponent));

	#elif (DIST_COMPENSATION == 2)
		exponent = 1.0/(3.0 -(0.3 * area * area) -(0.7 * area) + 0.3);
		return (1.0 - 0.9 * powl(1.0-area, exponent));

	#elif (DIST_COMPENSATION == 3)
		exponent = 1.0/(3.0 -(0.1 * area * area) -(1.05 * area) + 0.4);
		return (1.0 - 0.9 * powl(1.0-area, exponent));

	#elif (DIST_COMPENSATION == 4)
		exponent = 1.0/(3.0 -0.1 * powl(area, 3.0) + 0.27 * powl(area, 2.0) -1.4 * area + 0.5);
		return (1.0 - 0.9 * powl(1.0-area, exponent));

	#elif (DIST_COMPENSATION == 5)
		exponent = 1.0/(3.0 -0.26 * pow(area, 3.0) + 0.64 * pow(area, 2.0) -1.7 * area + 0.59);
		return (1.0 - 0.899999999999999999L * powl(1.0-area, exponent));

	#elif (DIST_COMPENSATION == 6)
		exponent = 1.0/(3.0 -0.27 * powl(area, 3.0) + 0.64 * powl(area, 2.0) -1.7 * area + 0.59);
		return (1.0 - 0.899999999999999999L * powl(1.0-area, exponent));

	#endif
	// These are hand made approximations.
}
#endif /* DIST_COMPENSATION */
/*------------------------------- linked list -------------------------------*/

struct llist	/* Linked list of unsigned short digits*/
{
	vamp_t value;
	struct llist *next;
};

struct llist *llist_init(vamp_t value , struct llist *next)
{
	struct llist *new = malloc(sizeof(struct llist));
	if (new == NULL)
		abort();

	new->value = value;
	new->next = next;
	return new;
}

void llist_free(struct llist *list)
{
	struct llist *tmp = list;
	for (struct llist *i = tmp; tmp != NULL; i = tmp) {
		tmp = tmp->next;
		free(i);
	}
}

#ifdef PRINT_RESULTS
void llist_print(struct llist *list, vamp_t count)
{
	for (struct llist *i = list; i != NULL ; i = i->next)
		printf("%llu %llu\n", ++count, i->value);
}
#endif
/*--------------------------- linked list handle  ---------------------------*/

struct llhandle
{
#ifdef STORE_RESULTS
	struct llist *head;
#endif
	vamp_t size;
};

struct llhandle *llhandle_init()
{
	struct llhandle *new = malloc(sizeof(struct llhandle));
	if (new == NULL)
		abort();

#ifdef STORE_RESULTS
	new->head = NULL;
#endif
	new->size = 0;
	return new;
}

void llhandle_free(struct llhandle *handle)
{
#ifdef STORE_RESULTS
	if (handle != NULL)
		llist_free(handle->head);
#endif
	free(handle);
}

void llhandle_add(struct llhandle *handle, [[maybe_unused]] vamp_t value)
{
	if (handle == NULL)
		return;

#ifdef STORE_RESULTS
	handle->head = llist_init(value, handle->head);
#endif
	handle->size += 1;
}

void llhandle_reset(struct llhandle *handle)
{
#ifdef STORE_RESULTS
	llist_free(handle->head);
	handle->head = NULL;
#endif
	handle->size = 0;
}

/*------------------------------- binary tree -------------------------------*/

struct btree
{
	struct btree *left;
	struct btree *right;
	vamp_t value;
	length_t height; //Should probably be less than 32
};

struct btree *btree_init(vamp_t value)
{
	struct btree *new = malloc(sizeof(struct btree));
	if (new == NULL)
		abort();

	new->left = NULL;
	new->right = NULL;
	new->height = 0;
	new->value = value;
	return new;
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
	sanitycheck(tree != NULL);
	tree->height = 0;
	if (tree->left != NULL && tree->left->height >= tree->height)
		tree->height = tree->left->height + 1;
	if (tree->right != NULL && tree->right->height >= tree->height)
		tree->height = tree->right->height + 1;
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
	sanitycheck(tree != NULL);
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
		return btree_init(node);
	}
	if (node == tree->value)
		return tree;
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

/*--------------------------- binary tree handle  ---------------------------*/

struct bthandle
{
	struct btree *tree;
	vamp_t size;
};

struct bthandle *bthandle_init()
{
	struct bthandle *new = malloc(sizeof(struct bthandle));
	if (new == NULL)
		abort();

	new->tree = NULL;
	new->size = 0;
	return new;
}

void bthandle_free(struct bthandle *handle)
{
	if (handle != NULL)
		btree_free(handle->tree);
	free(handle);
}

void bthandle_add(struct bthandle *handle, vamp_t number)
{
	sanitycheck(handle != NULL);
	handle->tree = btree_add(handle->tree, number, &(handle->size));
}

void bthandle_reset(struct bthandle *handle)
{
	btree_free(handle->tree);
	handle->tree = NULL;
	handle->size = 0;
}

/*
 * Move inactive data from binary tree to linked list
 * and free up memory. Works best with low thread counts.
 */
void bthandle_cleanup(
	[[maybe_unused]] struct bthandle *handle,
	[[maybe_unused]] vamp_t number,
	[[maybe_unused]] struct llhandle *lhandle)
{
#ifdef PROCESS_RESULTS
	struct btree *tree = handle->tree;
	vamp_t *size = &(handle->size);
	handle->tree = btree_cleanup(tree, number, lhandle, size);
#endif
}

/*----------------------------------- arr -----------------------------------*/

struct tile
{
	vamp_t lmin;
	vamp_t lmax;

#ifdef PROCESS_RESULTS
	struct llhandle *result;
	bool complete;
#endif
};

struct tile *tile_init(vamp_t min, vamp_t max)
{
	struct tile *new = malloc(sizeof(struct tile));
	if (new == NULL)
		abort();

	new->lmin = min;
	new->lmax = max;

#ifdef PROCESS_RESULTS
	new->result = NULL;
	new->complete = false;
#endif
	return new;
}

void tile_free(struct tile *ptr)
{
#ifdef PROCESS_RESULTS
	if (ptr != NULL)
		free(ptr->result);
#endif
	free(ptr);
}

/*--------------------------------- matrix  ---------------------------------*/

struct matrix
{
	struct tile **arr;
	vamp_t size;
	vamp_t unfinished;	// Current row to help iteration.
	vamp_t cleanup;
};

void matrix_set(struct matrix *ptr, vamp_t lmin, vamp_t lmax)
{
	sanitycheck(lmin <= lmax);
	#if (AUTO_TILE_SIZE && THREADS > 1)
		vamp_t tile_size = (lmax - lmin) / (4 * THREADS + 2);
	#else
		vamp_t tile_size = TILE_SIZE;
	#endif

	for (vamp_t i = 0; i < ptr->size; i++)
		tile_free(ptr->arr[i]);
	free(ptr->arr);

	ptr->unfinished = 0;
	ptr->cleanup = 0;
	ptr->size = div_roof((lmax - lmin + 1), tile_size + (tile_size < vamp_max));
	ptr->arr = malloc(sizeof(struct tile *) * ptr->size);
	if (ptr->arr == NULL)
		abort();

	#if DIST_COMPENSATION && (TILE_SIZE >= THREADS) && (THREADS > 1)
		vamp_t current = lmin;
		long double max;
		if (length(lmax) == length(vamp_max))
			max = vamp_max;
		else
			max = pow10v(length(lmax))-1;
		long double total_area = max - lmin + 1;
	#endif

	vamp_t x = 0;
	vamp_t iterator = tile_size;
	for (vamp_t i = lmin; i <= lmax; i += iterator + 1) {
		if (lmax - i < tile_size)
			iterator = lmax - i;

		vamp_t first = i;
		vamp_t last = i + iterator;

		#if DIST_COMPENSATION && (TILE_SIZE >= THREADS) && (THREADS > 1)
			long double area = ((long double)(last - lmin)) / total_area;
			vamp_t tmp = max * distribution_inverted_integral(area);
			first = current;
			if (tmp >= current && tmp <= last)
				last = tmp;
			current = last + 1;
		#endif

		ptr->arr[x++] = tile_init(first, last);

		if (i == lmax)
			break;
		if (i + iterator == vamp_max)
			break;
	}
}

struct matrix *matrix_init()
{
	struct matrix *new = malloc(sizeof(struct matrix));
	if (new == NULL)
		abort();

	new->arr = NULL;
	new->unfinished = 0;
	new->cleanup = 0;
	new->size = 0;
	return new;
}

void matrix_free(struct matrix *ptr)
{
	if (ptr == NULL)
		return;

	for (vamp_t i = 0; i < ptr->size; i++)
		tile_free(ptr->arr[i]);
	free(ptr->arr);
	free(ptr);
}

void matrix_print([[maybe_unused]] struct matrix *ptr, [[maybe_unused]] vamp_t *count)
{
#ifdef PRINT_RESULTS
	for (vamp_t x = ptr->cleanup; x < ptr->size; x++)
		if (ptr->arr[x] != NULL) {
			llist_print(ptr->arr[x]->result->head, *count);
			*count += ptr->arr[x]->result->size;
		}
#endif
}


/*-------------------------------- dig_count --------------------------------*/
struct dig_count
{
#if JENS_K_A_OPTIMIZATION
	digits_t *dig;
	fang_t digsize;
	fang_t power_a;
#endif
};

digits_t set_dig(fang_t number)
{
	digits_t ret = 0;
	for (fang_t i = number; i > 0; i /= 10) {
		digit_t digit = i % 10;
		if (digit >= DIGSKIP)
			ret += (digits_t)1 << ((digit - DIGSKIP) * DIGMULT);
	}
	return ret;
}

struct dig_count *dig_count_init([[maybe_unused]] vamp_t max)
{
	struct dig_count *new = NULL;
	#if JENS_K_A_OPTIMIZATION
		new = malloc(sizeof(struct dig_count));
		if (new == NULL)
			abort();

		fang_t length_a = length(max) / 3;
		fang_t length_b = length(max) - (2 * length_a);
		new->digsize = pow10v(length_b);

		if (length_a < 3)
			new->power_a = new->digsize;
		else
			new->power_a = pow10v(length_a);

		new->dig = malloc(sizeof(digits_t) * new->digsize);
		if (new->dig == NULL)
			abort();

		for (fang_t d = 0; d < new->digsize; d++) {
			new->dig[d] = set_dig(d);
		}
	#endif
	return new;
}

void dig_count_free(struct dig_count *ptr)
{
#if JENS_K_A_OPTIMIZATION
	free(ptr->dig);
	free(ptr);
#endif
}

/*---------------------------------------------------------------------------*/

struct vargs	/* Vampire arguments */
{
	vamp_t local_count;
	struct dig_count *digptr;

#ifdef PROCESS_RESULTS
	struct bthandle *thandle;
	struct llhandle *lhandle;
#endif
};

struct vargs *vargs_init(struct dig_count *digptr)
{
	struct vargs *new = malloc(sizeof(struct vargs));
	if (new == NULL)
		abort();

	new->local_count = 0;

#ifdef PROCESS_RESULTS
	new->lhandle = llhandle_init();
	new->thandle = bthandle_init();
#endif

	new->digptr = digptr;
	return new;
}

void vargs_free(struct vargs *args)
{
#ifdef PROCESS_RESULTS
	bthandle_free(args->thandle);
	llhandle_free(args->lhandle);
#endif
	free (args);
}

void vargs_reset(struct vargs *args)
{
	args->local_count = 0;

#ifdef PROCESS_RESULTS
	sanitycheck(args->lhandle == NULL);
	args->lhandle = llhandle_init();
	llhandle_reset(args->lhandle);
	bthandle_reset(args->thandle);
#endif
}

void vargs_btree_cleanup([[maybe_unused]] struct vargs *args, [[maybe_unused]] vamp_t number)
{
#ifdef PROCESS_RESULTS
	args->thandle->tree = btree_cleanup(args->thandle->tree, number, args->lhandle, &(args->thandle->size));
#endif
}

/*---------------------------------------------------------------------------*/

#if !JENS_K_A_OPTIMIZATION

void *vampire(vamp_t min, vamp_t max, struct vargs *args)
{
	length_t fang_length = length(min) / 2;

	vamp_t fmax;
	if (fang_length == length(fang_max))
		fmax = fang_max;
	else
		fmax = pow10v(fang_length); // Max factor value.

	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

	if (fmax < fang_max) {
		vamp_t fmaxsquare = fmax * fmax;
		if (max > fmaxsquare && min <= fmaxsquare)
			max = fmaxsquare; // Max can be bigger than fmax ^ 2: 9999 > 99 ^ 2.
	}

	for (vamp_t multiplier = fmax; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		vamp_t multiplicand = div_roof(min, multiplier); // fmin * fmax <= min - 10^n
		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier >= max_sqrt) {
			multiplicand_max = max / multiplier;
			// max can be less than (10^(n+1) -1)^2
		} else {
			multiplicand_max = multiplier;
			// multiplicand can be equal to multiplier:
			// 5267275776 = 72576 * 72576.
			if (mult_zero)
				vargs_btree_cleanup(args, (multiplier + 1) *  multiplier);
		}
		while (multiplicand <= multiplicand_max && con9(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand <= multiplicand_max) {
			vamp_t product_iterator = multiplier * 9; // <= 9 * 2^32
			vamp_t product = multiplier * multiplicand;

			length_t mult_array[10] = {0};
			for (fang_t i = multiplier; i != 0; i /= 10)
				mult_array[i % 10] += 1;

			for (; multiplicand <= multiplicand_max; multiplicand += 9) {
				uint16_t product_array[10] = {0};
				for (vamp_t p = product; p != 0; p /= 10)
					product_array[p % 10] += 1;

				for (digit_t i = 0; i < 10; i++)
				// Yes, we want to check all 10, this runs faster than checking only 8.
					if (product_array[i] < mult_array[i])
						goto vampire_exit;

				digit_t temp;
				for (fang_t m = multiplicand; m != 0; m /= 10) {
					temp = m % 10;
					if (product_array[temp] < 1)
						goto vampire_exit;
					else
						product_array[temp]--;
				}
				for (digit_t i = 0; i < 8; i++)
					if (product_array[i] != mult_array[i])
						goto vampire_exit;

				if (mult_zero || notrailingzero(multiplicand)) {
					#if defined COUNT_RESULTS ||  defined DUMP_RESULTS
						args->local_count += 1;
					#endif
					#ifdef DUMP_RESULTS
						printf("%llu=%llu*%llu\n", product, multiplier, multiplicand);
					#endif
					#ifdef PROCESS_RESULTS
						bthandle_add(args->thandle, product);
					#endif
				}
vampire_exit:
				product += product_iterator;
			}
		}
	}
	vargs_btree_cleanup(args, 0);
	return 0;
}

#else /* !JENS_K_A_OPTIMIZATION */

void *vampire(vamp_t min, vamp_t max, struct vargs *args)
{
	length_t fang_length = length(min) / 2;

	vamp_t fmax;
	if (fang_length == length(fang_max))
		fmax = fang_max;
	else
		fmax = pow10v(fang_length); // Max factor value.

	fang_t min_sqrt = sqrtv_roof(min);
	fang_t max_sqrt = sqrtv_floor(max);

	if (fmax < fang_max) {
		vamp_t fmaxsquare = fmax * fmax;
		if (max > fmaxsquare && min <= fmaxsquare)
			max = fmaxsquare; // Max can be bigger than fmax ^ 2: 9999 > 99 ^ 2.
	}

	fang_t power_a = args->digptr->power_a;
	digits_t *dig = args->digptr->dig;

	for (vamp_t multiplier = fmax; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		vamp_t multiplicand = div_roof(min, multiplier); // fmin * fmax <= min - 10^n
		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier >= max_sqrt) {
			multiplicand_max = max / multiplier;
			// max can be less than (10^(n+1) -1)^2
		} else {
			multiplicand_max = multiplier;
			// multiplicand can be equal to multiplier:
			// 5267275776 = 72576 * 72576.

			if (mult_zero)
				vargs_btree_cleanup(args, (multiplier + 1) *  multiplier);

		}
		while (multiplicand <= multiplicand_max && con9(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand <= multiplicand_max) {
			vamp_t product_iterator = multiplier * 9; // <= 9 * 2^32
			vamp_t product = multiplier;
			product *= multiplicand; // avoid overflow

			fang_t step0 = product_iterator % power_a;
			fang_t step1 = product_iterator / power_a; // 90 <= step1 < 900

			fang_t e0 = multiplicand % power_a; // e0 < 10 ^ (n - 1)
			fang_t e1 = multiplicand / power_a; // e1 < 100

			/*
			 * digd = dig[multiplier];
			 * Each digd is calculated and accessed only once, we don't need to store them in memory.
			 * We can calculate digd on the spot and make the dig array 10 times smaller.
			 */

			digits_t digd;

			if (min_sqrt >= args->digptr->digsize)
				digd = set_dig(multiplier);
			else
				digd = dig[multiplier];

			fang_t de0 = product % power_a;
			fang_t de1 = (product / power_a) % power_a;
			fang_t de2 = ((product / power_a) / power_a); // 10^3 <= de2 < 10^4

			for (; multiplicand <= multiplicand_max; multiplicand += 9) {
				if (digd + dig[e0] + dig[e1] == dig[de0] + dig[de1] + dig[de2])
					if (mult_zero || notrailingzero(multiplicand)) {
					#if defined COUNT_RESULTS ||  defined DUMP_RESULTS
						args->local_count += 1;
					#endif
					#ifdef DUMP_RESULTS
						//printf("%llu=%llu*%llu\n", product, multiplier, multiplicand);
						printf("%llu\n", product);
					#endif
					#ifdef PROCESS_RESULTS
						bthandle_add(args->thandle, product);
					#endif
					}
				e0 += 9;
				if (e0 >= power_a) {
					e0 -= power_a;
					e1 ++;
				}
				de0 += step0;
				if (de0 >= power_a) {
					de0 -= power_a;
					de1 += 1;
				}
				de1 += step1;
				if (de1 >= power_a) {
					de1 -= power_a;
					de2 += 1;
				}
				product += product_iterator;
			}
			if (multiplier < max_sqrt && mult_zero)
				vargs_btree_cleanup(args, product);
		}
	}
	vargs_btree_cleanup(args, 0);
	return 0;
}
#endif  /* !JENS_K_A_OPTIMIZATION */

/*--------------------------------- Threads ---------------------------------*/
struct thread_args
{
	pthread_mutex_t *mutex;
	struct matrix *mat;
	vamp_t *count;
	vamp_t local_count;
	double	runtime;
	struct dig_count *digptr;

#ifdef SPDT_CLK_MODE
	struct timespec start;
#endif
};

struct thread_args *thread_args_init(
	pthread_mutex_t *mutex,
	struct matrix *mat,
	vamp_t *count,
	struct dig_count *digptr)
{
	struct thread_args *new = malloc(sizeof(struct thread_args));
	if (new == NULL)
		abort();

	new->mutex = mutex;
	new->mat = mat;
	new->count = count;
	new->local_count = 0;
	new->runtime = 0.0;
	new->digptr = digptr;
	return new;
}

void thread_args_free(struct thread_args *ptr)
{
	free(ptr);
}

void thread_timer_start([[maybe_unused]] struct thread_args *ptr)
{
#ifdef SPDT_CLK_MODE
	clock_gettime(SPDT_CLK_MODE, &(ptr->start));
#endif
}

void thread_timer_stop([[maybe_unused]] struct thread_args *ptr)
{
#ifdef SPDT_CLK_MODE
	struct timespec finish;
	clock_gettime(SPDT_CLK_MODE, &(finish));
	double elapsed = (finish.tv_sec - ptr->start.tv_sec);
	elapsed += (finish.tv_nsec - ptr->start.tv_nsec) / 1000000000.0;
	ptr->runtime = elapsed;
#endif
}

void *thread_worker(void *void_args)
{
	struct thread_args *args = (struct thread_args *)void_args;
	struct vargs *vamp_args = vargs_init(args->digptr);
	//thread_timer_start(args);

	struct tile *current;
	bool active = true;

	while (active) {
		active = false;

// Critical section start
		pthread_mutex_lock(args->mutex);

		if (args->mat->unfinished < args->mat->size) {
			current = args->mat->arr[args->mat->unfinished];
			active = true;
			args->mat->unfinished += 1;
		}
		pthread_mutex_unlock(args->mutex);
// Critical section end

		if (active) {
			vampire(current->lmin, current->lmax, vamp_args);

// Critical section start
			pthread_mutex_lock(args->mutex);
				#ifdef PROCESS_RESULTS
					bool row_complete = false;
					current->result = vamp_args->lhandle;
					current->complete = true;
					do {
						row_complete = false;
						if (args->mat->cleanup < args->mat->size)
							row_complete = args->mat->arr[args->mat->cleanup]->complete;

						if (row_complete == true) {
							#ifdef PRINT_RESULTS
								llist_print(args->mat->arr[args->mat->cleanup]->result->head, *(args->count));
							#endif
							*(args->count) += args->mat->arr[args->mat->cleanup]->result->size;

							tile_free(args->mat->arr[args->mat->cleanup]);
							args->mat->arr[args->mat->cleanup] = NULL;
							args->mat->cleanup += 1;
						}

					} while (row_complete == true);
				#else
					*(args->count) += vamp_args->local_count;
				#endif
			pthread_mutex_unlock(args->mutex);
// Critical section end

			args->local_count += vamp_args->local_count;
			vargs_reset(vamp_args);
		}
	}
	vargs_free(vamp_args);
	//thread_timer_stop(args);
	return 0;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
	if (argc != 3) {
		if (argc == 2) {
			if (strcmp(argv[1], "--version") == 0) {
				printf("Helsing 1.0, a vampire number generator.\n");
				print_licence();
			}
		} else {
			printf("Usage: helsing [min] [max] | [OPTIONS]\n");
			printf("\nArguments:\n");
			printf("  --version                  \
				output version information and exit\n");
		}
		return 0;
	}

	bool err = false;
	vamp_t min = atov(argv[1], &err);
	if (err) {
		fprintf(stderr, "Min out of range: [0, %llu]\n", vamp_max);
		return 1;
	}
	vamp_t max = atov(argv[2], &err);
	if (err) {
		fprintf(stderr, "Max out of range: [0, %llu]\n", vamp_max);
		return 1;
	}
	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		return 1;
	}
	if (max > 9999999999999999) {
		fprintf(stderr, "WARNING: the code might experience overflow,");
		fprintf(stderr, " set DIG_ELEMENT_BITS to 64\n");
	}

	min = get_min(min, max);
	max = get_max(min, max);

	// local min, max for use inside loop
	vamp_t lmin = min;
	vamp_t lmax = get_lmax(lmin, max);

	struct matrix *mat = matrix_init();
	struct thread_args *tinput[THREADS];

	pthread_t threads[THREADS];
	vamp_t counter = 0;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	struct dig_count *digptr = dig_count_init(max);

	for (thread_t thread = 0; thread < THREADS; thread++)
		tinput[thread] = thread_args_init(&mutex, mat, &counter, digptr);

	for (; lmax <= max;) {
		fprintf(stderr, "Checking range: [%llu, %llu]\n", lmin, lmax);
		matrix_set(mat, lmin, lmax);
		for (thread_t thread = 0; thread < THREADS; thread++)
			assert(pthread_create(&threads[thread], NULL, thread_worker, (void *)tinput[thread]) == 0);
		for (thread_t thread = 0; thread < THREADS; thread++)
			pthread_join(threads[thread], 0);

		matrix_print(mat, &counter);
		if (lmax == max)
			break;

		lmin = get_min(lmax + 1, max);
		lmax = get_lmax(lmin, max);
	}
	dig_count_free(digptr);
	matrix_free(mat);

	#ifdef SPDT_CLK_MODE
		double total_time = 0.0;
		fprintf(stderr, "Thread  Runtime Count\n");
		for (thread_t thread = 0; thread<THREADS; thread++) {
			fprintf(stderr, "%u\t%.2lfs\t%llu\n", thread, tinput[thread]->runtime, tinput[thread]->local_count);
			total_time += tinput[thread]->runtime;
		}
		fprintf(stderr, "\nFang search took: %.2lfs, average: %.2lfs\n", total_time, total_time / THREADS);
	#endif

	#if (defined PROCESS_RESULTS) && PRINT_DIST_MATRIX && (THREADS > 8)
		double distrubution = 0.0;
		for (thread_t thread = 0; thread < THREADS; thread++) {
			distrubution += ((double)tinput[thread]->count) / ((double)(counter));
			fprintf(stderr, "(%lf,0.1+%lu/%lu),", distrubution, thread+1, (10 * THREADS) / 9);
			if ((thread+1) % 10 == 0)
				fprintf(stderr, "\n");
		}
	#endif

	#if defined COUNT_RESULTS ||  defined DUMP_RESULTS
		fprintf(stderr, "Found: %llu valid fang pairs.\n", counter);
	#else
		fprintf(stderr, "Found: %llu vampire numbers.\n", counter);
	#endif

	for (thread_t thread = 0; thread<THREADS; thread++)
		thread_args_free(tinput[thread]);

	return 0;
}
