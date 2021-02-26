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
#include <math.h>
#include <ctype.h>

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
 * Based on results I produced a function that estimates the distribution
 * of vampire numbers. The integral of the inverse can help with load
 * distribution between threads, minimizing the need for load balancing
 * and its overhead.
 */

#define DIST_COMPENSATION true
#define PRINT_DIST_MATRIX false

/*
 * ITERATOR:
 *
 * Default value: 10000000000000000000ULL (10 ^ 19)
 * Maximum value: 18446744073709551615ULL (2^64 -1)
 *
 * Every (at most) #ITERATOR numbers:
 *  -results are collected, processed and optionally printed
 *  -allocated memory for vampire number storage is freed
 *  -new work is assigned to threads
 *
 * ITERATOR should be set as unsigned long long (ULL extension at the end).
 *
 * You can optimize the code for specific [min,max] runs, to use less memory
 * by setting ITERATOR below max. A good balance between performance loss
 * and memory usage is max/100.
 */

#define ITERATOR 18446744073709551615ULL

/*
 * JENS_K_A_OPTIMIZATION:
 *
 * WARNING: Depending on the max value, this optimization may require more
 * memory than the system has available.
 *
 * Enables a code optimization written by Jens Kruse Andersen.
 * The code allocates a 2D array and stores some data, then performs
 * simple operations (comparison, addition, subtraction).
 *
 * This way the code doesn't perform expensive operations such as
 * multiplication, division, modulo.
 *
 * Without any further tweaks, it seems like this algorithm doesn't scale
 * very well. I did notice that performance above 10^16 was not as good.
 * My best guess is that the 2D array gets too big and the memory latency
 * penalty to access it hinders performance. Also the dig[] array might 
 * experience overflow that may affect the accuracy of results.
 *
 * http://primerecords.dk/vampires/index.htm
 */

#define JENS_K_A_OPTIMIZATION true

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
 * Both vamp_t and fang_t must be unsigned, vamp_t should be double the size of
 * fang_t and fang_max, vamp_max should be set accordingly.
 *
 * You should be able to change vamp_t up to 256-bit without any issues.
 * If you want to go any higher check the uint8_t for overflow.
*/

typedef unsigned long long vamp_t;
#define vamp_max ULLONG_MAX

typedef unsigned long fang_t;
#define fang_max ULONG_MAX

/*--------------------------- PREPROCESSOR_STUFF  ---------------------------*/

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

/*---------------------------- ULLONG FUNCTIONS  ----------------------------*/

uint8_t length(vamp_t number)
{
	uint8_t length = 0;
	for (; number > 0; number /= 10)
		length++;
	return length;
}

bool length_isodd(vamp_t number)
{
	return (length(number) % 2);
}

// pow10 for vampire type.
vamp_t pow10v(uint8_t exponent)
{
	assert(exponent <= length(vamp_max) - 1);
	vamp_t number = 1;
	for (; exponent > 0; exponent--)
		number *= 10;
	return number;
}

bool willoverflow(vamp_t number, uint8_t digit)
{
	assert(digit < 10);
	if (number > vamp_max / 10)
		return true;
	if (number == vamp_max / 10 && digit > vamp_max % 10)
		return true;
	return false;
}

// ASCII to vampire type
vamp_t atoull(const char *str, bool *error)
{
	vamp_t number = 0;
	for (uint8_t i = 0; isdigit(str[i]); i++){
		if (willoverflow(number, str[i] - '0')) {
			*error = true;
			return 1;
		}
		number = 10 * number + str[i] - '0';
	}
	return number;
}

bool notrailingzero(fang_t number)
{
	return ((number % 10) != 0);
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
fang_t sqrtv(vamp_t number)
{
	vamp_t root = number >> 1; // Initial estimate
	fang_t ret = number;
	if (root) { // Sanity check
		vamp_t x1 = (root + number / root) / 2; // Update
		while (x1 < root) { // This also checks for cycle
			root = x1;
			x1 = (root + number / root) / 2;
		}
		ret = root;
	}
	return ret;
}

/*---------------------------------- llist ----------------------------------*/

struct llist	/* Linked list of unsigned short digits*/
{
	vamp_t number;
	struct llist *next;
};

struct llist *llist_init(vamp_t number , struct llist *next)
{
	struct llist *new = malloc(sizeof(struct llist));
	assert(new != NULL);

	new->number = number;
	new->next = next;
	return new;
}

int llist_free(struct llist *llist_ptr)
{
	struct llist *current = llist_ptr;
	for (struct llist *temp = current; current != NULL ;) {
		temp = current;
		current = current->next;
		free(temp);
	}
	return 0;
}

unsigned long long llist_print(struct llist *llist_ptr, unsigned long long count)
{
	for (struct llist *i = llist_ptr; i != NULL ; i = i->next) {
		count++;
#if OEIS_OUTPUT
		printf("%llu %llu\n", count, i->number);
#endif
	}
	return count;
}

/*--------------------------------- LLHEAD  ---------------------------------*/

struct llhead
{
	struct llist *first;
	struct llist *last;
};

struct llhead *llhead_init(struct llist *first, struct llist *last)
{
	struct llhead *new = malloc(sizeof(struct llhead));
	assert(new != NULL);
	new->first = first;
	if (last != NULL)
		new->last = last;
	else
		new->last = new->first;
	return (new);
}

int llhead_free(struct llhead *llhead_ptr)
{
	if (llhead_ptr != NULL)
		llist_free(llhead_ptr->first);
	free(llhead_ptr);
	return 0;
}

/*-------------------------------- ULLBTREE  --------------------------------*/

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

int ullbtree_free(struct ullbtree *tree)
{
	if (tree != NULL) {
		if (tree->left != NULL)
			ullbtree_free(tree->left);
		if (tree->right != NULL)
			ullbtree_free(tree->right);
	}
	free(tree);
	return 0;
}

unsigned long long ullbtree_results(struct ullbtree *tree, unsigned long long i)
{
	if (tree !=NULL) {
		i = ullbtree_results(tree->left, i);

		i++;
#if OEIS_OUTPUT
		printf("%llu %llu\n", i, tree->value);
#endif
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
	if(tree->right != NULL){
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
	if(tree->left != NULL){
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
	if (node == tree->value) {
		return tree;
	}
	else if (node < tree->value) {
		tree->left = ullbtree_add(tree->left, node, count);
	} else {
		tree->right = ullbtree_add(tree->right, node, count);
	}
	ullbtree_reset_height(tree);
	tree = ullbtree_balance(tree);
	return tree;
}
/*
struct ullbtree *ullbtree_add(
	struct ullbtree *tree,
	vamp_t node,
	unsigned long long *count)
{
	if(tree == NULL){
		return  ullbtree_init(node);
		*count += 1;
	}


	struct ullbtree *arr[32] = {NULL};
	bool arrb[32] = {0};
	uint8_t i = 0;
	struct ullbtree *current = tree;
	for (; current != NULL; i++) {
		arr[i]=current;
		if (node < current->value) {
			current = current->left;
			arrb[i] = 0;
		}
		else if (node > current->value){
			current = current->right;
			arrb[i] = 1;
		}
		else
			return tree;
	}
	i--;
	if (node < arr[i]->value)
		arr[i]->left = ullbtree_init(node);
	else
		arr[i]->right = ullbtree_init(node);
	
	*count += 1;

	for (; i > 0; i--) {
		ullbtree_reset_height(arr[i]);

		if(arrb[i-1] == 0)
			arr[i-1]->left = ullbtree_balance(arr[i]);
		else
			arr[i-1]->right = ullbtree_balance(arr[i]);

	}
	ullbtree_reset_height(arr[0]);
	tree = ullbtree_balance(arr[0]);

	return tree;
}
*/
struct ullbtree *ullbtree_cleanup(
	struct ullbtree *tree,
	vamp_t number,
	struct llhead *ll)
{
	if (tree == NULL)
		return NULL;
	tree->right = ullbtree_cleanup(tree->right, number, ll);

	if (tree->value >= number) {
		//printf("%llu\n", tree->value);
		ll->first = llist_init(tree->value, ll->first);
		if (ll->first->next == NULL)
			ll->last = ll->first;

		struct ullbtree *temp = tree->left;
		tree->left = NULL;
		ullbtree_free(tree);

		tree = temp;
		tree = ullbtree_cleanup(tree, number, ll);
	}

	if (tree == NULL){
		return NULL;
	}
	ullbtree_reset_height(tree);
	tree = ullbtree_balance(tree);

	return tree;
}

/*---------------------------------------------------------------------------*/

typedef struct vargs	/* Vampire arguments */
{
	vamp_t min;
	vamp_t max;
	unsigned long long count;
	double	runtime;

#if !defined DUMP_RESULTS
	struct ullbtree *result;
	struct llhead *llresult;
#endif

#if JENS_K_A_OPTIMIZATION
	vamp_t*dig;
#endif
} vargs;

vargs *vargs_init(vamp_t min, vamp_t max)
{
	vargs *new = malloc(sizeof(vargs));
	assert(new != NULL);
	new->min = min;
	new->max = max;
	new->count = 0;
	new->runtime = 0.0;

#if !defined DUMP_RESULTS
	new->result = NULL;
	new->llresult = llhead_init(NULL, NULL);
#endif
	return new;
}

int vargs_free(vargs *vargs_ptr)
{
#if !defined DUMP_RESULTS
	ullbtree_free(vargs_ptr->result);
	llhead_free(vargs_ptr->llresult);
#endif
	free (vargs_ptr);
	return 0;
}

double distribution_inverted_integral(double area)
{
	return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0-(0.7*area))));
	// This is a hand made approximation.
}

thread_t vargs_split(
	vargs *args[],
	vamp_t min,
	vamp_t max)
{
	thread_t current = 0;
	do {
		args[current]->min = min;
		args[current]->max = (max - min) / (THREADS - current) + min;
		min = args[current]->max + 1;
	} while (args[current++]->max < max);
	current--;

#if DIST_COMPENSATION
	double area;
	vamp_t temp;
	for (thread_t i = 0; i < current; i++) {
		area = ((double)i+1)/((double)THREADS);
		temp = (double)max * distribution_inverted_integral(area);

		if (temp > args[i]->min && temp < args[i]->max) {
			args[i]->max = temp;
			args[i+1]->min = temp + 1;
		}
	}
#endif

	args[current]->max = max;
	return (current);
}

// Modulo 9 lack of congruence
bool con9(fang_t multiplier, fang_t multiplicand)
{
	return ((multiplier + multiplicand) % 9
		!= (multiplier * multiplicand) % 9);
}

/*---------------------------------------------------------------------------*/

void *vampire(void *void_args)
{
#ifdef SPDT_CLK_MODE
	struct timespec start, finish;
	double elapsed;
	clock_gettime(SPDT_CLK_MODE, &start);
#endif

	vargs *args = (vargs *)void_args;

	vamp_t min = args->min;
	vamp_t max = args->max;

	//Min Max range for both factors
	fang_t factor_max = pow10v((length(max) / 2)) -1;
	//fang_t factor_min = pow10v((length(min) / 2) - 1);

	if (max > factor_max * factor_max && min <= factor_max * factor_max)
		max = factor_max * factor_max;
	// Max can be bigger than factor_max ^ 2: 9999 > 99 ^ 2.

	fang_t min_sqrt = min / sqrtv(min);
	fang_t max_sqrt = max / sqrtv(max);

	#if (JENS_K_A_OPTIMIZATION)
		fang_t power10 = pow10v((length(min) / 2) - 2);
		if (power10 < 900)
			power10 = 1000;

		vamp_t *dig = args->dig;
		//vamp_t digd;

		//fang_t step0;
		//uint16_t step1; // 90 <= step1 < 900
	
		//fang_t de0,de1;
		//uint16_t de2; // 10^3 <= de2 < 10^4

		//fang_t e0;
		//uint8_t e1;
	#endif

	for (fang_t multiplier = factor_max; multiplier >= min_sqrt; multiplier--) {
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
				args->result = ullbtree_cleanup(args->result, (multiplier+1) * multiplier, args->llresult);
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

				fang_t e0 = multiplicand % power10;
				uint8_t e1 = multiplicand / power10;

				/*
				 * digd = dig[multiplier];
				 * Each digd is calculated and accessed only once, we don't need to store them in memory.
				 * We can calculate digd on the spot and make the dig array 10 times smaller.
				 */

				vamp_t digd = 0;
				fang_t d0 = multiplier;
				uint8_t digitsmax = length(product);
				for (uint8_t i = 0; i < digitsmax; i++) {
					digd += ((vamp_t)1 << ((d0 % 10) * 4));
					d0 /= 10;
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

				if ((mult_zero || notrailingzero(multiplicand))){
#if !defined DUMP_RESULTS
					args->result = ullbtree_add(args->result, product, &(args->count));
					//args->llresult->first = llist_init(product, args->llresult->first);
#else
					printf("%llu = %lu %lu\n", product, multiplier, multiplicand);
#endif
				}
#if !(JENS_K_A_OPTIMIZATION)
vampire_exit:
#endif
				product += product_iterator;

				#if (JENS_K_A_OPTIMIZATION)
					e0 += 9;
					if (e0 >= power10) {
						e0 -= power10;
						e1 ++;
					}
					de0 += step0;
					if (de0 >= power10){
						de0 -= power10;
						de1 += 1;
					}
					de1 += step1;
					if (de1 >= power10){
						de1 -= power10;
						de2 += 1;
					}
				#endif	
			}
			//args->result = ullbtree_cleanup(args->result, multiplier * multiplier, args->llresult);

		}
	}

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
		fprintf(stderr, "Input out of range: [0, %llu]\n", vamp_max);
		return 1;
	}
	vamp_t max = atoull(argv[2], &err);
	if (err) {
		fprintf(stderr, "Maximum value overflows the limit: %llu\n", vamp_max);
		return 1;
	}

	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		return 0;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	// local min, max for use inside loop
	vamp_t lmin = min;
	vamp_t lmax = get_lmax(lmin, max);

	vargs *input[THREADS];
	thread_t thread;

	for (thread = 0; thread < THREADS; thread++)
		input[thread] = vargs_init(0, 0);

	pthread_t threads[THREADS];
	unsigned long long iterator = ITERATOR;
	thread_t active_threads = THREADS;
#if !defined DUMP_RESULTS
	unsigned long long result = 0;
#endif

	for (; lmax <= max;) {
		fprintf(stderr, "Checking range: [%llu, %llu]\n", lmin, lmax);

#if JENS_K_A_OPTIMIZATION
		uint8_t digsize = length(lmin) / 2 - 1;
		if (digsize <= 2)
			digsize = 3;

		fang_t factor_max = pow10v(digsize);

		vamp_t *dig = malloc(sizeof(vamp_t) * factor_max);
		assert(dig != NULL);

		for (fang_t d = 0; d < factor_max; d++) {
			dig[d] = 0;
			fang_t d0 = d;
			for (uint8_t i = 0; i < length(factor_max); i++) {
				uint8_t digit = d0 % 10;
				dig[d] += ((vamp_t)1 << (digit * 4));
				d0 /= 10;
			}
		}

		for (thread = 0; thread < active_threads; thread++)
			input[thread]->dig = dig;
#endif

		iterator = ITERATOR;
		for (vamp_t i = lmin; i <= lmax; i += iterator + 1) {
			if (lmax - i < ITERATOR)
				iterator = lmax - i;

			active_threads = vargs_split(input, i, i + iterator) + 1;

			for (thread = 0; thread < active_threads; thread++) {
				assert(pthread_create(&threads[thread], NULL, vampire, (void *)input[thread]) == 0);
			}
			for (thread = 0; thread < active_threads; thread++) {
				pthread_join(threads[thread], 0);
#if !defined DUMP_RESULTS

				result = ullbtree_results(input[thread]->result, result);
				result = llist_print(input[thread]->llresult->first, result);
				ullbtree_free(input[thread]->result);
				llist_free(input[thread]->llresult->first);
				input[thread]->llresult->first = NULL;
				input[thread]->result = NULL;
#endif
			}
			if(i == lmax)
				break;
			if(i + iterator == vamp_max)
				break;
		}
//-------------------------------------
#if JENS_K_A_OPTIMIZATION
			free(input[0]->dig);
#endif
//-------------------------------------
		if (lmax == max)
			break;

		lmin = get_min (lmax + 1, max); 
		// lmax + 1 <= vamp_max, because lmax <= max and lmax != max.
		lmax = get_lmax(lmin, max);
	}

#ifdef SPDT_CLK_MODE
	double total_time = 0.0;
	fprintf(stderr, "Thread  Count   Runtime\n");
	for (thread = 0; thread<THREADS; thread++) {
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
	for (thread = 0; thread < THREADS; thread++) {
		distrubution += ((double)input[thread]->count) / ((double)(result));
		fprintf(stderr, "(1+%u/%u,%lf),", thread+1, THREADS/9, distrubution);
		if ((thread+1) % 10 == 0)
			fprintf(stderr, "\n");
	}
#endif
	for (thread = 0; thread<THREADS; thread++) {
		vargs_free(input[thread]);
	}
	pthread_exit(NULL);
	return 0;
}
