/*
 * Copyright (c) 2021, Pierro Zachareas, et al.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

// Compile with: gcc -O3 -Wall -Wextra -pthread -lm -o helsing helsing.c
// Check memory with valgrind --tool=massif

/*--------------------------- COMPILATION OPTIONS ---------------------------*/

/*
 * THREADS:
 *
 * Default value: 1
 *
 * Affects how many processing threads will spawn.
 * (note: thread count above #cores may not improve performance)
 */

#define THREADS 16
#define thread_t uint16_t

/*
 * DIST_COMPENSATION:
 *
 * 	Based on results, I produced a function that tries to estimate the
 * distribution of vampire numbers. The integral of the inverse can help with
 * load distribution between threads, minimizing the need for load balancing
 * and its overhead.
 */

#define DIST_COMPENSATION true
#define PRINT_DIST_MATRIX false

/*
 * ITERATOR:
 *
 * Maximum value: 18446744073709551615ULL (2^64 -1)
 *
 * Every (at most) #ITERATOR numbers three things happen:
 *  1) Results are collected, processed and optionally printed
 *  2) Allocated memory for vampire number storage is freed
 *  3) New work is assigned to threads
 *
 * ITERATOR should be set as unsigned long long (ULL extension at the end).
 *
 * 	You can optimize the code for specific [min,max] runs, to use less
 * memory by setting ITERATOR below max. A good balance between performance
 * loss and memory usage is max/100.
 */

#define ITERATOR 100000000000000ULL

/*
 * JENS_K_A_OPTIMIZATION:
 *
 * 	This code was originally written by Jens Kruse Andersen, and is
 * included with their permission. Adjustments were made to accomodate
 * features such as multithreading.
 *
 * Source: http://primerecords.dk/vampires/index.htm
 *
 * 	The code allocates an array, where each element is n bits long.
 *
 * 	Each element has a position, an array key; for example in the array
 * "array[]" the element "array[1]" has the array key 1.
 *
 * 	The value of each element is a concatenation of (n/8)-bit unsigned 
 * integers that hold the total count of each digit (except 0s & 1s) of its
 * array key. For example a 32-bit sized element gets split into 8 * 4-bit
 * segments and a 64-bit sized element gets split into 8 * 8-bit segments:
 *                                     [77776666|55554444|33332222|99998888]
 * [77777777|66666666|55555555|44444444|33333333|22222222|99999999|88888888]
 * 64       56       48       40       32       24       16       8        0
 *
 * 	We can choose any digit and ignore it, I chose not to store the 0s.
 * I think I can get away with not storing 1s too; Only a fang with more than
 * 8 * 1s could fool the modulo 9 congruence and such numbers are pretty rare.
 * The 9s & 8s are positioned before the rest, because modulo 8 is faster than
 * subtraction.
 *
 * 	This way the code doesn't perform expensive operations such as modulo,
 * multiplication, division, for values that are already stored in the array.
 *
 * 	If the array gets too big, the memory latency penalty to access it
 * might hinder performance. Also the dig[] array will experience overflow
 * above (10 ^ (2 ^ n/8) - 1) - 1. To check numbers above 10^15 - 1 set
 * DIG_ELEMENT_BITS to 64.
 */

#define JENS_K_A_OPTIMIZATION true
#define DIG_ELEMENT_BITS 32

/*
 * OEIS_OUTPUT:
 *
 * Prints out all the vampire numbers ordered and numbered.
 * Redirect the output to a text file. (vampire min max > vampirenumbers.txt)
 */

#define OEIS_OUTPUT false

/*
 * MEASURE_RUNTIME:
 *
 * Measures the total runtime of each thread and prints it at the end.
 */

#define MEASURE_RUNTIME false
#define PRINT_VAMPIRE_COUNT true

/*
 * SANITY_CHECK:
 *
 * Performs extra checks to catch runtime problems. (Currently does nothing)
 */

#define SANITY_CHECK false

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

/*--------------------------- PREPROCESSOR_STUFF  ---------------------------*/
#if DIG_ELEMENT_BITS == 64
	typedef uint64_t dig_t;
#elif DIG_ELEMENT_BITS == 32
	typedef uint32_t dig_t;
#endif

#define DIGMULT (DIG_ELEMENT_BITS/8)

#if MEASURE_RUNTIME
	#if defined(CLOCK_MONOTONIC)
		#define SPDT_CLK_MODE CLOCK_MONOTONIC
	#elif defined(CLOCK_REALTIME)
		#define SPDT_CLK_MODE CLOCK_REALTIME
	#endif
#endif

#if !OEIS_OUTPUT
	//#define DUMP_RESULTS
#endif

#ifdef DUMP_RESULTS
	#ifdef PRINT_VAMPIRE_COUNT
	#undef PRINT_VAMPIRE_COUNT
	#endif

	#define PRINT_VAMPIRE_COUNT false
#endif

/*---------------------------- HELPER FUNCTIONS  ----------------------------*/

uint8_t length(vamp_t x)
{
	uint8_t length = 0;
	for (; x > 0; x /= 10)
		length++;
	return length;
}

bool length_isodd(vamp_t x)
{
	return (length(x) % 2);
}

// pow10 for vampire type.
vamp_t pow10v(uint8_t exponent)
{
	assert(exponent <= length(vamp_max) - 1);
	vamp_t base = 1;
	for (; exponent > 0; exponent--)
		base *= 10;
	return base;
}

bool willoverflow(vamp_t x, uint8_t digit)
{
	assert(digit < 10);
	if (x > vamp_max / 10)
		return true;
	if (x == vamp_max / 10 && digit > vamp_max % 10)
		return true;
	return false;
}

// ASCII to vampire type
vamp_t atoull(const char *str, bool *err)
{
	vamp_t number = 0;
	for (uint8_t i = 0; isdigit(str[i]); i++) {
		if (willoverflow(number, str[i] - '0')) {
			*err = true;
			return 1;
		}
		number = 10 * number + str[i] - '0';
	}
	return number;
}

bool notrailingzero(fang_t x)
{
	return ((x % 10) != 0);
}

vamp_t get_min(vamp_t min, vamp_t max)
{
	if (length_isodd(min)) {
		uint8_t min_length = length(min);
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
		uint8_t max_length = length(max);
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
fang_t sqrtv(vamp_t x)
{
	vamp_t root = x >> 1; // Initial estimate
	fang_t ret = x;
	if (root) { // Sanity check
		vamp_t tmp = (root + x / root) / 2; // Update
		while (tmp < root) { // This also checks for cycle
			root = tmp;
			tmp = (root + x / root) / 2;
		}
		ret = root;
	}
	return ret;
}

// Modulo 9 lack of congruence
bool con9(fang_t x, fang_t y)
{
	return ((x + y) % 9 != (x * y) % 9);
}

/*------------------------------- linked list -------------------------------*/

struct llist	/* Linked list of unsigned short digits*/
{
	vamp_t value;
	struct llist *next;
};

struct llist *llist_init(vamp_t value , struct llist *next)
{
	struct llist *new = malloc(sizeof(struct llist));
	assert(new != NULL);

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

void llist_print(struct llist *list, unsigned long long count)
{
	for (struct llist *i = list; i != NULL ; i = i->next)
		printf("%llu %llu\n", ++count, i->value);
}

/*--------------------------- linked list handle  ---------------------------*/

struct llhandle
{
	struct llist *head;
	vamp_t size;
};

struct llhandle *llhandle_init(struct llist *head)
{
	struct llhandle *new = malloc(sizeof(struct llhandle));
	assert(new != NULL);
	new->size = 0;
	new->head = head;

	for (struct llist *i = head; i != NULL ; i = i->next) {
		assert(new->size != vamp_max);
		new->size += 1;
	}

	return new;
}

void llhandle_free(struct llhandle *handle)
{
	if (handle != NULL)
		llist_free(handle->head);
	free(handle);
}

void llhandle_add(struct llhandle *handle, vamp_t value)
{
	if (handle == NULL)
		return;

	handle->head = llist_init(value, handle->head);
	handle->size += 1;
}

void llhandle_reset(struct llhandle *handle)
{
	llist_free(handle->head);
	handle->head = NULL;
	handle->size = 0;
}

/*------------------------------- binary tree -------------------------------*/

struct ullbtree
{
	struct ullbtree *left;
	struct ullbtree *right;
	vamp_t value;
	uint8_t height; //Should probably be less than 32
};

struct ullbtree *ullbtree_init(vamp_t value)
{
	struct ullbtree *new = malloc(sizeof(struct ullbtree));
	assert(new != NULL);

	new->left = NULL;
	new->right = NULL;
	new->height = 0;
	new->value = value;
	return new;
}

void ullbtree_free(struct ullbtree *tree)
{
	if (tree != NULL) {
		if (tree->left != NULL)
			ullbtree_free(tree->left);
		if (tree->right != NULL)
			ullbtree_free(tree->right);
	}
	free(tree);
}

unsigned long long ullbtree_results(struct ullbtree *tree, unsigned long long i)
{
	if (tree !=NULL) {
		i = ullbtree_results(tree->left, i);
		printf("%llu %llu\n", ++i, tree->value);
		i = ullbtree_results(tree->right, i);
	}
	return i;
}

int is_balanced(struct ullbtree *tree)
{
	//assert (tree != NULL);
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

void ullbtree_reset_height(struct ullbtree *tree)
{
	//assert(tree != NULL);
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

struct ullbtree *ullbtree_rotate_l(struct ullbtree *tree)
{
	//assert(tree != NULL);
	//assert(tree->right != NULL);
	if (tree->right != NULL) {
		struct ullbtree *right = tree->right;
		tree->right = right->left;
		ullbtree_reset_height(tree);
		right->left = tree;
		ullbtree_reset_height(right);
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

struct ullbtree *ullbtree_rotate_r(struct ullbtree *tree)
{
	//assert(tree != NULL);
	//assert(tree->left != NULL);
	if (tree->left != NULL) {
		struct ullbtree *left = tree->left;
		tree->left = left->right;
		ullbtree_reset_height(tree);
		left->right = tree;
		ullbtree_reset_height(left);
		return left;
	}
	return tree;
}

struct ullbtree *ullbtree_balance(struct ullbtree *tree)
{
	//assert(tree != NULL);
	int isbalanced = is_balanced(tree);
	if (isbalanced > 1) {
		if (is_balanced(tree->left) < 0) {
			tree->left = ullbtree_rotate_l(tree->left);
			ullbtree_reset_height(tree); //maybe optional?
		}
		tree = ullbtree_rotate_r(tree);
	}
	else if (isbalanced < -1) {
		if (is_balanced(tree->right) > 0) {
			tree->right = ullbtree_rotate_r(tree->right);
			ullbtree_reset_height(tree); //maybe optional?
		}
		tree = ullbtree_rotate_l(tree);

	}
	return tree;
}

struct ullbtree *ullbtree_add(
	struct ullbtree *tree,
	vamp_t node,
	unsigned long long *count)
{
	if (tree == NULL) {
		*count += 1;
		return ullbtree_init(node);
	}
	if (node == tree->value)
		return tree;
	else if (node < tree->value)
		tree->left = ullbtree_add(tree->left, node, count);
	else
		tree->right = ullbtree_add(tree->right, node, count);

	ullbtree_reset_height(tree);
	tree = ullbtree_balance(tree);
	return tree;
}

struct ullbtree *ullbtree_cleanup(
	struct ullbtree *tree,
	vamp_t number,
	struct llhandle *lhandle,
	unsigned long long *btree_size)
{
	if (tree == NULL)
		return NULL;
	tree->right = ullbtree_cleanup(tree->right, number, lhandle, btree_size);

	if (tree->value >= number) {
		llhandle_add(lhandle, tree->value);

		struct ullbtree *tmp = tree->left;
		tree->left = NULL;
		ullbtree_free(tree);
		*btree_size -= 1;

		//tree = tmp;
		//tree = ullbtree_cleanup(tree, number, lhandle, btree_size);
		tree = ullbtree_cleanup(tmp, number, lhandle, btree_size);
	}

	if (tree == NULL)
		return NULL;
	ullbtree_reset_height(tree);
	tree = ullbtree_balance(tree);

	return tree;
}

/*--------------------------- binary tree handle  ---------------------------*/

struct bthandle
{
	struct ullbtree *tree;
	vamp_t size;
};

struct bthandle *bthandle_init()
{
	struct bthandle *new = malloc(sizeof(struct bthandle));
	new->tree = NULL;
	new->size = 0;
	return new;
}

void bthandle_free(struct bthandle *handle)
{
	if(handle != NULL)
		ullbtree_free(handle->tree);
	free(handle);
}

void bthandle_add(struct bthandle *handle, vamp_t number)
{
	assert(handle != NULL);
	handle->tree = ullbtree_add(handle->tree, number, &(handle->size));
}

void bthandle_reset(struct bthandle *handle)
{
	ullbtree_free(handle->tree);
	handle->tree = NULL;
	handle->size = 0;
}

void bthandle_cleanup(struct bthandle *handle, vamp_t number, struct llhandle *ll)
{
	handle->tree = ullbtree_cleanup(handle->tree, number, ll, &(handle->size));
}

/*---------------------------------------------------------------------------*/

struct vargs	/* Vampire arguments */
{
	vamp_t min;
	vamp_t max;
	unsigned long long count;
	double	runtime;

#if !defined DUMP_RESULTS
	struct bthandle *thandle;
	struct llhandle *lhandle;
#endif

#if JENS_K_A_OPTIMIZATION
	dig_t *dig;
	fang_t digsize;
#endif
};

struct vargs *vargs_init(vamp_t min, vamp_t max)
{
	struct vargs *new = malloc(sizeof(struct vargs));
	assert(new != NULL);
	new->min = min;
	new->max = max;
	new->count = 0;
	new->runtime = 0.0;

#if !defined DUMP_RESULTS
	new->lhandle = llhandle_init(NULL);
	new->thandle = bthandle_init();
#endif
	return new;
}

void vargs_free(struct vargs *args)
{
#if !defined DUMP_RESULTS
	bthandle_free(args->thandle);
	llhandle_free(args->lhandle);
#endif
	free (args);
}

double distribution_inverted_integral(double area)
{

	/*
	* old:
	* real	14m20.802s
	* user	213m32.842s
	* sys	0m13.075s
	*/
	return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0-(0.7*area))));

	/*
	* new:
	* real	13m53.228s
	* user	209m37.086s
	* sys	0m14.265s
	*/
	return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0 -(0.3 * area * area) -(0.7 * area) + 0.3)));

	/*
	* new new:
	* real	14m0.453s
	* user	210m42.727s
	* sys	0m12.523s
	*/
	//return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0 -(0.1 * area * area) -(1.05 * area) + 0.4)));

	/*
	* new new new:
	* real	14m10.593s
	* user	211m4.023s
	* sys	0m13.791s
	*/
	//return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0 -0.1 * pow(area, 3.0) + 0.27 * pow(area, 2.0) -1.4 * area + 0.5)));

	/*
	* new new new new:
	* real	14m5.800s
	* user	210m42.727s
	* sys	0m13.952s
	*/
	//double exponent = 1.0/(3.0 -0.26 * pow(area, 3.0) + 0.64 * pow(area, 2.0) -1.7 * area + 0.59);
	//return (1.0 - 0.899 * pow(1.0-area, exponent));
	// This is a hand made approximation.
}

thread_t vargs_split(
	struct vargs *args[],
	vamp_t min,
	vamp_t max)
{
	thread_t current = 0;
	do {
		args[current]->min = min;
		args[current]->max = (max - min) / (THREADS - current) + min;

#if DIST_COMPENSATION
		double area = ((double)current + 1) / ((double)THREADS);
		vamp_t temp = (double)max * distribution_inverted_integral(area);
		if (temp > args[current]->min && temp < args[current]->max)
			args[current]->max = temp;
#endif
		min = args[current]->max + 1;
		//printf("[%llu %llu]\n", args[current]->min, args[current]->max);
	} while (args[current++]->max < max);
	current--;

	args[current]->max = max;
	return (current);
}

vamp_t vargs_results(struct vargs *args, vamp_t result)
{
#if OEIS_OUTPUT
	ullbtree_results(args->thandle->tree, result);
#endif
	result += args->thandle->size;

#if OEIS_OUTPUT
	llist_print(args->lhandle->head, result);
#endif
	result += args->lhandle->size;

	args->count += args->thandle->size + args->lhandle->size;

	llhandle_reset(args->lhandle);
	bthandle_reset(args->thandle);
	return result;
}

/*---------------------------------------------------------------------------*/

void *vampire(void *void_args)
{
#ifdef SPDT_CLK_MODE
	struct timespec start, finish;
	double elapsed;
	clock_gettime(SPDT_CLK_MODE, &start);
#endif

	struct vargs *args = (struct vargs *)void_args;
	vamp_t min = args->min;
	vamp_t max = args->max;
	fang_t fmax = pow10v(length(max) / 2); // Max factor value.

	/*
	 * armhf was giving me errors, because unsigned long was 32-bit,
	 * while on my x86_64 pc unsigned long was 64-bit.
	 */
	vamp_t fmaxsquare = (vamp_t)fmax * fmax;

	if (max > fmaxsquare && min <= fmaxsquare)
		max = fmaxsquare; // Max can be bigger than fmax ^ 2: 9999 > 99 ^ 2.

	fang_t min_sqrt = min / sqrtv(min);
	fang_t max_sqrt = max / sqrtv(max);

	#if (JENS_K_A_OPTIMIZATION)
		fang_t power10 = pow10v((length(min) / 2)) / 100;
		if (power10 < 900)
			power10 = 1000;

		dig_t *dig = args->dig;
	#endif

	for (fang_t multiplier = fmax; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		fang_t multiplicand = min / multiplier + !!(min % multiplier);
		// fmin * fmax <= min - 10^n

		bool mult_zero = notrailingzero(multiplier);

		fang_t multiplicand_max;
		if (multiplier >= max_sqrt) {
			multiplicand_max = max / multiplier;
			// max can be less than (10^(n+1) -1)^2
		} else {
			multiplicand_max = multiplier;
			// multiplicand can be equal to multiplier:
			// 5267275776 = 72576 * 72576.

#if !defined DUMP_RESULTS
			if (mult_zero)
				bthandle_cleanup(args->thandle, (vamp_t)(multiplier+1) * multiplier, args->lhandle);
			/*
			 * Move inactive data from binary tree to linked list
			 * and free up memory. Works best with low thread counts.
			 */
#endif
		}
		while (multiplicand <= multiplicand_max && con9(multiplier, multiplicand))
			multiplicand++;

		if (multiplicand <= multiplicand_max) {
			//mult_zero = notrailingzero(multiplier);

			vamp_t product_iterator = multiplier * 9; // <= 9 * 2^32
			vamp_t product = multiplier * multiplicand;

			#if (JENS_K_A_OPTIMIZATION)
				fang_t step0 = product_iterator % power10;
				uint16_t step1 = product_iterator / power10; // 90 <= step1 < 900

				fang_t e0 = multiplicand % power10; // e0 < 1000
				uint8_t e1 = multiplicand / power10; // e1 < 10

				/*
				 * digd = dig[multiplier];
				 * Each digd is calculated and accessed only once, we don't need to store them in memory.
				 * We can calculate digd on the spot and make the dig array 10 times smaller.
				 */

				dig_t digd;

				if(min_sqrt >= args->digsize) {
					digd = 0;
					for (fang_t i = multiplier; i > 0; i /= 10) {
						uint8_t digit = i % 10;
						if(digit > 1)
							digd += ((vamp_t)1 << ((digit % 8) * DIGMULT)); 
					}
				}
				else {
					digd = dig[multiplier];
				}

				fang_t de0 = product % power10;
				fang_t de1 = (product / power10) % power10;
				uint16_t de2 = (product / power10) / power10; // 10^3 <= de2 < 10^4
			#else
				uint8_t mult_array[10] = {0};
				for (fang_t i = multiplier; i != 0; i /= 10)
					mult_array[i % 10] += 1;
			#endif

			for (;multiplicand <= multiplicand_max; multiplicand += 9) {
				#if (JENS_K_A_OPTIMIZATION)
					if (digd + dig[e0] + dig[e1] == dig[de0] + dig[de1] + dig[de2])
				#else
					uint16_t product_array[10] = {0};
					for (vamp_t p = product; p != 0; p /= 10)
						product_array[p % 10] += 1;

					for (uint8_t i = 0; i < 10; i++)
					// Yes, we want to check all 10, this runs faster than checking only 8.
						if (product_array[i] < mult_array[i])
							goto vampire_exit;

					uint8_t temp;
					for (fang_t m = multiplicand; m != 0; m /= 10) {
						temp = m % 10;
						if (product_array[temp] < 1)
							goto vampire_exit;
						else
							product_array[temp]--;
					}
					for (uint8_t i = 0; i < 8; i++)
						if (product_array[i] != mult_array[i])
							goto vampire_exit;
				#endif

				if (mult_zero || notrailingzero(multiplicand)) {
#if !defined DUMP_RESULTS
					bthandle_add(args->thandle, product);
#else
					printf("%llu = %lu %lu\n", product, multiplier, multiplicand);
#endif
				}
				#if (JENS_K_A_OPTIMIZATION)
					e0 += 9;
					if (e0 >= power10) {
						e0 -= power10;
						e1 ++;
					}
					de0 += step0;
					if (de0 >= power10) {
						de0 -= power10;
						de1 += 1;
					}
					de1 += step1;
					if (de1 >= power10) {
						de1 -= power10;
						de2 += 1;
					}
				#else
vampire_exit:
				#endif
				product += product_iterator;
			}
			//args->result = ullbtree_cleanup(args->result, multiplier * multiplier, args->lhandle, &(args->btree_size));

		}
	}
	bthandle_cleanup(args->thandle, 0, args->lhandle);

#ifdef SPDT_CLK_MODE
	clock_gettime(SPDT_CLK_MODE, &finish);
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	args->runtime += elapsed;
#endif

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		fprintf(stderr, "A vampire number generator\n");
		fprintf(stderr, "Usage: vampire [min] [max]\n");
		return 0;
	}

	bool err = false;
	vamp_t min = atoull(argv[1], &err);
	if (err) {
		fprintf(stderr, "Min out of range: [0, %llu]\n", vamp_max);
		return 1;
	}
	vamp_t max = atoull(argv[2], &err);
	if (err) {
		fprintf(stderr, "Max out of range: [0, %llu]\n", vamp_max);
		return 1;
	}

	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		return 1;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	// local min, max for use inside loop
	vamp_t lmin = min;
	vamp_t lmax = get_lmax(lmin, max);

	struct vargs *input[THREADS];

	for (thread_t thread = 0; thread < THREADS; thread++)
		input[thread] = vargs_init(0, 0);

	pthread_t threads[THREADS];
	unsigned long long iterator = ITERATOR;
	thread_t active_threads = THREADS;

#if !defined DUMP_RESULTS
	unsigned long long result = 0;
#endif

#if JENS_K_A_OPTIMIZATION
	fang_t digsize = pow10v(length(max) / 2 - 1);
	if (digsize < 1000)
		digsize = 1000;
	dig_t *dig = malloc(sizeof(dig_t) * digsize);
	assert(dig != NULL);

	for (fang_t d = 0; d < digsize; d++) {
		dig[d] = 0;
		for (fang_t i = d; i > 0; i /= 10) {
			uint8_t digit = i % 10;
			if(digit > 1)
				dig[d] += ((dig_t)1 << ((digit % 8) * DIGMULT));
		}
	}

	for (thread_t thread = 0; thread < THREADS; thread++){
		input[thread]->dig = dig;
		input[thread]->digsize = digsize;
	}
#endif
	for (; lmax <= max;) {
		fprintf(stderr, "Checking range: [%llu, %llu]\n", lmin, lmax);
		iterator = ITERATOR;
		for (vamp_t i = lmin; i <= lmax; i += iterator + 1) {
			if (lmax - i < ITERATOR)
				iterator = lmax - i;

			active_threads = vargs_split(input, i, i + iterator) + 1;

			for (thread_t thread = 0; thread < active_threads; thread++) {
				assert(pthread_create(&threads[thread], NULL, vampire, (void *)input[thread]) == 0);
			}
			for (thread_t thread = 0; thread < active_threads; thread++) {
				pthread_join(threads[thread], 0);
#if !defined DUMP_RESULTS
				result = vargs_results(input[thread], result);
#endif
			}
			if (i == lmax)
				break;
			if (i + iterator == vamp_max)
				break;
		}
		if (lmax == max)
			break;

		lmin = get_min (lmax + 1, max);
		// lmax + 1 <= vamp_max, because lmax <= max and lmax != max.
		lmax = get_lmax(lmin, max);
	}

//-------------------------------------
#if JENS_K_A_OPTIMIZATION
		free(input[0]->dig);
#endif
//-------------------------------------

#ifdef SPDT_CLK_MODE
	double total_time = 0.0;
	fprintf(stderr, "Thread  Count   Runtime\n");
	for (thread_t thread = 0; thread<THREADS; thread++) {
		fprintf(stderr, "%u\t%llu\t%lf\t[%llu\t%llu]\n", thread, input[thread]->count, input[thread]->runtime, input[thread]->min, input[thread]->max);
		total_time += input[thread]->runtime;
	}
	fprintf(stderr, "\nFang search took: %lf, average: %lf\n", total_time, total_time / THREADS);
#endif

#if PRINT_VAMPIRE_COUNT
	fprintf(stderr, "Found: %llu vampire numbers.\n", result);
#endif

#if (PRINT_DIST_MATRIX) && (THREADS > 8)
	double distrubution = 0.0;
	for (thread_t thread = 0; thread < THREADS; thread++) {
		distrubution += ((double)input[thread]->count) / ((double)(result));
		//fprintf(stderr, "(1+%u/%u,%lf),", thread+1, THREADS/9, distrubution);
		fprintf(stderr, "(%lf,0.1+%u/%u),", distrubution, thread+1, 10*THREADS/9);
		if ((thread+1) % 10 == 0)
			fprintf(stderr, "\n");
	}
#endif
	for (thread_t thread = 0; thread<THREADS; thread++) {
		vargs_free(input[thread]);
	}
	pthread_exit(NULL);
	return 0;
}
