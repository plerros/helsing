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

/*--------------------------- COMPILATION OPTIONS ---------------------------*/

/*
 * NUM_THREADS:
 *
 * Affects how many processing threads will spawn.
 * (note: thread count above #cores may not improve performance)
 */

#define NUM_THREADS 16

/*
 * DISTRIBUTION_COMPENSATION:
 *
 * Based on results I produced a function that estimates the distribution
 * of vampire numbers. The integral of the inverse can help with load
 * distribution between threads, minimizing the need for load balancing
 * and its overhead.
 */

#define DISTRIBUTION_COMPENSATION
//#define PRINT_DISTRIBUTION_MATRIX

/*
 * ITERATOR:
 *
 * Default value: 10000000000000000000ULL
 * Maximum value: 18446744073709551616ULL (2^64 - 1)
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

#define ITERATOR 10000000000000000000ULL

/*
 * JENS_KRUSSE_ANDERSEN_OPTIMIZATION:
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
 * penalty to access it hinders performance.
 *
 * http://primerecords.dk/vampires/index.htm
 */

#define JENS_KRUSSE_ANDERSEN_OPTIMIZATION

/*
 * OEIS_OUTPUT:
 *
 * Prints out all the vampire numbers ordered and numbered.
 * Redirect the output to a text file. (vampire min max > vampirenumbers.txt)
 */

//#define OEIS_OUTPUT

/*
 * MEASURE_RUNTIME:
 *
 * Measures the total runtime of each thread and prints it at the end.
 */

#define MEASURE_RUNTIME

#define PRINT_VAMPIRE_COUNT

/*
 * SANITY_CHECK:
 *
 * Performs extra checks to catch runtime problems.
*/

//#define SANITY_CHECK

/*--------------------------- PREPROCESSOR_STUFF  ---------------------------*/

#ifdef MEASURE_RUNTIME
	#if defined(CLOCK_MONOTONIC)
		#define SPDT_CLK_MODE CLOCK_MONOTONIC
	#elif defined(CLOCK_REALTIME)
		#define SPDT_CLK_MODE CLOCK_REALTIME
	#endif
#endif

/*---------------------------- ULLONG FUNCTIONS  ----------------------------*/

uint8_t length(unsigned long long number)
{
	uint8_t length = 0;
	for (; number > 0; number /= 10)
		length++;
	return length;
}

bool length_isodd(unsigned long long number)
{
	return (length(number) % 2);
}

unsigned long long pow10ull(uint8_t exponent)
{
	assert(exponent <= length(ULLONG_MAX) - 1);
	unsigned long long number = 1;
	for (; exponent > 0; exponent--)
		number *= 10;
	return number;
}

bool willoverflow(unsigned long long number, uint8_t digit)
{
	assert(digit < 10);
	if (number > ULLONG_MAX / 10)
		return true;
	if (number == ULLONG_MAX / 10 && digit > ULLONG_MAX % 10)
		return true;
	return false;
}

// ASCII to unsigned long long
unsigned long long atoull(const char *str, bool *error)
{
	unsigned long long number = 0;
	for (uint8_t i = 0; isdigit(str[i]); i++){
		if (willoverflow(number, str[i] - '0')) {
			*error = true;
			return 1;
		}
		number = 10 * number + str[i] - '0';
	}
	return number;
}

bool notrailingzero(unsigned long number)
{
	return ((number % 10) != 0);
}

unsigned long long get_min(unsigned long long min, unsigned long long max)
{
	if (length_isodd(min)) {
		uint8_t min_length = length(min);
		if (min_length < length(max))
			min = pow10ull(min_length);
		else
			min = max;
	}
	return min;
}

unsigned long long get_max(unsigned long long min, unsigned long long max)
{
	if (length_isodd(max)) {
		uint8_t max_length = length(max);
		if (max_length > length(min))
			max = pow10ull(max_length - 1) - 1;
		else
			max = min;
	}
	return max;
}

unsigned long long get_lmax(unsigned long long lmin, unsigned long long max)
{
	unsigned long long lmax;
	if (length(lmin) < length(ULLONG_MAX)) {
		lmax = pow10ull(length(lmin)) - 1;
		if (lmax < max)
			return lmax;
	}
	return max;
}

unsigned long long sqrtull(unsigned long long number)
{
	unsigned long long root = number >> 1; // Initial estimate
	if (root) { // Sanity check
		unsigned long long x1 = (root + number / root) >> 1; // Update
		while (x1 < root) { // This also checks for cycle
			root = x1;
			x1 = (root + number / root) >> 1;
		}
		return root;
	}
	return number;
}

/*---------------------------------- llist ----------------------------------*/

typedef struct llist	/* Linked list of unsigned short digits*/
{
	unsigned long long number;
	struct llist *next;
} llist;

llist *llist_init(unsigned long long number , llist *next)
{
	llist *new = malloc(sizeof(llist));
	assert(new != NULL);

	new->number = number;
	new->next = next;
	return new;
}

int llist_free(llist *llist_ptr)
{
	llist *current = llist_ptr;
	for (llist *temp = current; current != NULL ;) {
		temp = current;
		current = current->next;
		free(temp);
	}
	return 0;
}

unsigned long long llist_print(llist *llist_ptr, unsigned long long count)
{
	for (llist *i = llist_ptr; i != NULL ; i = i->next) {
		count++;
#if defined(OEIS_OUTPUT)
		printf("%llu %llu\n", count, i->number);
#endif
	}
	return count;
}

/*--------------------------------- LLHEAD  ---------------------------------*/

typedef struct llhead
{
	struct llist *first;
	struct llist *last;
} llhead;

llhead *llhead_init(llist *first, llist *last)
{
	llhead *new = malloc(sizeof(llhead));
	assert(new != NULL);
	new->first = first;
	if (last != NULL)
		new->last = last;
	else
		new->last = new->first;
	return (new);
}

int llhead_free(llhead *llhead_ptr)
{
	if (llhead_ptr != NULL)
		llist_free(llhead_ptr->first);
	free(llhead_ptr);
	return 0;
}

/*-------------------------------- ULLBTREE  --------------------------------*/

typedef struct ullbtree
{
	struct ullbtree *left;
	struct ullbtree *right;
	unsigned long long value;
	uint8_t height; //Should probably be less than 32
} ullbtree;

ullbtree *ullbtree_init(unsigned long long value)
{
	ullbtree *new = malloc(sizeof(ullbtree));
	assert(new != NULL);

	new->left = NULL;
	new->right = NULL;
	new->height = 0;
	new->value = value;
	return new;
}

int ullbtree_free(ullbtree *tree)
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

unsigned long long ullbtree_results(ullbtree *tree, unsigned long long i)
{
	if (tree !=NULL) {
		i = ullbtree_results(tree->left, i);

		i++;
#if defined(OEIS_OUTPUT)
		printf("%llu %llu\n", i, tree->value);
#endif
		i = ullbtree_results(tree->right, i);
	}
	return i;
}

int is_balanced(ullbtree *tree)
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

void ullbtree_reset_height(ullbtree *tree)
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

ullbtree *ullbtree_rotate_l(ullbtree *tree)
{
	//assert(tree != NULL);
	//assert(tree->right != NULL);
	if(tree->right != NULL){
		ullbtree *right = tree->right;
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

ullbtree *ullbtree_rotate_r(ullbtree *tree)
{
	//assert(tree != NULL);
	//assert(tree->left != NULL);
	if(tree->left != NULL){
		ullbtree *left = tree->left;
		tree->left = left->right;
		ullbtree_reset_height(tree);
		left->right = tree;
		ullbtree_reset_height(left);
		return left;
	}
	return tree;
}

ullbtree *ullbtree_balance(ullbtree *tree)
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

ullbtree *ullbtree_add(
	ullbtree *tree,
	unsigned long long node,
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

ullbtree *ullbtree_cleanup(
	ullbtree *tree,
	unsigned long long number,
	llhead *ll)
{
	if (tree == NULL){
		return NULL;
	}
	tree->right = ullbtree_cleanup(tree->right, number, ll);

	if (tree->value >= number) {
		//printf("%llu\n", tree->value);
		ll->first = llist_init(tree->value, ll->first);
		if (ll->first->next == NULL)
			ll->last = ll->first;

		ullbtree *temp = tree->left;
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
	unsigned long long min;
	unsigned long long max;
	unsigned long long count;
	double	runtime;
	ullbtree *result;
	llhead *llresult;

#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
	unsigned long *dig;
#endif
} vargs;

vargs *vargs_init(unsigned long long min, unsigned long long max)
{
	vargs *new = malloc(sizeof(vargs));
	assert(new != NULL);
	new->min = min;
	new->max = max;
	new->count = 0;
	new->runtime = 0.0;
	new->result = NULL;
	new->llresult = llhead_init(NULL, NULL);
	return new;
}

int vargs_free(vargs *vargs_ptr)
{
	ullbtree_free(vargs_ptr->result);
	llhead_free(vargs_ptr->llresult);
	free (vargs_ptr);
	return 0;
}

double distribution_inverted_integral(double area)
{
	return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0-(0.7*area))));
	//This is a hand made approximation.
}

uint16_t vargs_split(
	vargs *args[],
	unsigned long long min,
	unsigned long long max)
{
	uint16_t current = 0;

	args[current]->min = min;
	args[current]->max = (max-min)/(NUM_THREADS - current) + min;

	for (; args[current]->max < max;) {
		min = args[current]->max + 1;
		current ++;
		args[current]->min = min;
		args[current]->max = (max-min)/(NUM_THREADS - current) + min;
	}

#ifdef DISTRIBUTION_COMPENSATION
	//unsigned long long factor_min = pow10ull((length(min)) - 1);
	//unsigned long long factor_max = pow10ull((length(max))) -1;
	unsigned long long temp;
	for (uint16_t i = 0; i < current; i++) {
		temp = (double)max * distribution_inverted_integral(((double)i+1)/((double)NUM_THREADS));

		if (temp > args[i]->min && temp < args[i]->max) {
			args[i]->max = temp;
			args[i+1]->min = temp + 1;
		}
	}
#endif

	args[current]->max = max;
	return (current);
}

/*----------------------------------------------------------------------------*/

void *vampire(void *void_args)
{
#ifdef SPDT_CLK_MODE
	struct timespec start, finish;
	double elapsed;
	clock_gettime(SPDT_CLK_MODE, &start);
#endif

	vargs *args = (vargs *)void_args;

	unsigned long long min = args->min;
	unsigned long long max = args->max;

	//Min Max range for both factors
	unsigned long long factor_min = pow10ull((length(min) / 2) - 1);
	unsigned long factor_max = pow10ull((length(max) / 2)) -1;

	if (max >= factor_max * factor_max)
		max = factor_max * factor_max;

	unsigned long min_sqrt = min / sqrtull(min);
	unsigned long max_sqrt = max / sqrtull(max);

	//Adjust range for Factors
	unsigned long multiplicant;
	unsigned long multiplier = factor_max;
	unsigned long multiplicant_max;
	unsigned long long product_iterator; // < 10^n

	unsigned long long product;
	bool mult_zero;

//-------------------------------------
#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
	unsigned long power10 = factor_min / 10;
	unsigned long *dig = args->dig;

	unsigned long step0;
	unsigned long step1;
	unsigned long de0,de1,de2;
	unsigned long e0,e1;
	unsigned long digd;
#endif
//-------------------------------------

	//printf("min max [%llu, %llu], fmin fmax [%llu, %llu], %llu\n", min, max, factor_min, factor_max, multiplier);
	for (; multiplier >= min_sqrt; multiplier--) {
		if (multiplier % 3 == 1)
			continue;

		multiplicant = min/multiplier + !!(min % multiplier); // fmin * fmax <= min - 10^n

		if (multiplier >= max_sqrt) {
			multiplicant_max = max/multiplier; //max can be less than (10^(n+1) -1)^2
		} else {
			multiplicant_max = multiplier; // multiplicant can be equal to multiplier, 5267275776 = 72576 * 72576.
			args->result = ullbtree_cleanup(args->result, (multiplier+1) * (multiplier), args->llresult);
			//Move inactive data from binary tree to linked list to free up memory. Works best with low thread counts.
		}

		if (multiplicant <= multiplicant_max) {

#ifndef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
			uint8_t mult_array[10] = {0};
			for (unsigned long i = multiplier; i != 0; i /= 10)
				mult_array[i % 10] += 1;
#endif
			mult_zero = notrailingzero(multiplier);

			unsigned long A_1 = multiplier - 1;
			unsigned long long AB_A_B = A_1 * (multiplicant - 1) - 1;
			for (unsigned long long CA_1 = 0; (multiplicant <= multiplicant_max) && ((AB_A_B + CA_1) % 9 != 0); CA_1 += A_1)
				multiplicant++;

			product_iterator = multiplier * 9;
			product = multiplier * (multiplicant);


//-------------------------------------
#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
			step0 = product_iterator % power10;
			step1 = product_iterator / power10;

			e0 = multiplicant % power10;
			e1 = multiplicant / power10;
			digd = dig[multiplier];

			de0 = product % power10;
			de1 = (product / power10) % power10;
			de2 = (product / power10) / power10;
#endif
//-------------------------------------

			for (;multiplicant <= multiplicant_max; multiplicant += 9) {

#ifndef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
				uint16_t product_array[10] = {0};
				for (unsigned long p = product; p != 0; p /= 10)
					product_array[p % 10] += 1;
				for (uint8_t i = 0; i < 10; i++) { // Yes, we want to check all 10, this is faster than only checking 8.
					if (product_array[i] < mult_array[i])
						goto vampire_exit;
				}

				uint8_t temp;
				for (unsigned long m = multiplicant; m != 0; m /= 10) {
					temp = m % 10;
					if (product_array[temp] < 1)
						goto vampire_exit;
					else
						product_array[temp]--;
				}
				for (uint8_t i = 0; i < 8; i++) {
					if (product_array[i] != mult_array[i])
						goto vampire_exit;
				}
#endif

//-------------------------------------
#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
				if (digd + dig[e1] + dig[e0] != dig[de0] + dig[de1] + dig[de2])
					goto vampire_exit;
#endif
//-------------------------------------

				if ((mult_zero || notrailingzero(multiplicant)))
					args->result = ullbtree_add(args->result, product, &(args->count));
vampire_exit:
				product += product_iterator;

//-------------------------------------
#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
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
//-------------------------------------

			}
		}
	}

	//args->result = ullbtree_cleanup(args->result, 0, args->llresult);
	//Move inactive data from binary tree to linked list to free up memory. Works best with low thread counts.

#ifdef SPDT_CLK_MODE
	clock_gettime(SPDT_CLK_MODE, &finish);
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	args->runtime += elapsed;
	//printf("%llu \t%llu \t%llu\tin %lf\n", min, max, counter, elapsed);
#endif

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		fprintf(stderr, "A vampire number generator\nUsage: vampire [min] [max]\n");
		return 0;
	}

	bool error = 0;
	unsigned long long min = atoull(argv[1], &error);
	if (error) {
		fprintf(stderr, "Minimum value overflows the limit: %llu\n", ULLONG_MAX);
		return 1;
	}
	unsigned long long max = atoull(argv[2], &error);
	if (error) {
		fprintf(stderr, "Maximum value overflows the limit: %llu\n", ULLONG_MAX);
		return 1;
	}

	if (min > max) {
		fprintf(stderr, "Invalid arguments, min <= max\n");
		return 0;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	// local min, max for use inside loop
	unsigned long long lmin = min;
	unsigned long long lmax = get_lmax(lmin, max);

	vargs *input[NUM_THREADS];
	uint16_t thread;

	for (thread = 0; thread < NUM_THREADS; thread++) {
		input[thread] = vargs_init(0, 0);
	}

	pthread_t threads[NUM_THREADS];
	unsigned long long iterator = ITERATOR;
	uint16_t active_threads = NUM_THREADS;
	int returncode;

	unsigned long long result = 0;

	for (; lmax <= max;) {

		fprintf(stderr, "Checking range: [%llu, %llu]\n", lmin, lmax);

//-------------------------------------
#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
		unsigned long digitsmax = length(lmin);
		unsigned long factor_max = pow10ull((length(lmin) / 2)) - 1;
		unsigned long *dig = malloc(sizeof(unsigned long) * (factor_max + 1));
		assert(dig != NULL);

		unsigned long d,d0,digit;

		for (d = 0; d <= factor_max; d++) {
			dig[d] = 0;
			d0 = d;
			for (unsigned long i = 0; i < digitsmax; i++) {
				digit = d0 % 10;
				//dig64[d] += ((unsigned long long)1 << (digit * 4));
				dig[d] += ((unsigned long long)1 << (digit * 3)); // was *4 with 64-bit dig
				d0 /= 10;
			}
		}

		for (thread = 0; thread < active_threads; thread++) {
			input[thread]->dig = dig;
		}
#endif
//-------------------------------------

		iterator = ITERATOR;
		for (unsigned long long i = lmin; i <= lmax; i += iterator + 1) {
			if (lmax - i < ITERATOR)
				iterator = lmax - i;

			active_threads = vargs_split(input, i, i + iterator) + 1;

			for (thread = 0; thread < active_threads; thread++) {
				returncode = pthread_create(&threads[thread], NULL, vampire, (void *)input[thread]);
				if (returncode) {
					fprintf(stderr, "Error: unable to create thread %d\n", returncode);
					return 1;
				}
			}
			for (thread = 0; thread < active_threads; thread++) {
				pthread_join(threads[thread], 0);
				result = ullbtree_results(input[thread]->result, result);
				//result += input[thread]->count;
				result = llist_print(input[thread]->llresult->first, result);
				ullbtree_free(input[thread]->result);
				llist_free(input[thread]->llresult->first);
				input[thread]->llresult->first = NULL;
				input[thread]->result = NULL;
			}
		}
//-------------------------------------
#ifdef JENS_KRUSSE_ANDERSEN_OPTIMIZATION
			free(input[0]->dig);
#endif
//-------------------------------------
		if (lmax != max) {
			lmin = get_min (lmax + 1, max); // lmax + 1 <= ULLONG_MAX, because lmax < max.
			lmax = get_lmax(lmin, max);


		} else {
			break;
		}
	}

#ifdef SPDT_CLK_MODE
	double total_time = 0.0;
	fprintf(stderr, "Thread  Count   Runtime\n");
	for (thread = 0; thread<NUM_THREADS; thread++) {
		fprintf(stderr, "%u\t%llu\t%lf\t[%llu\t%llu]\n", thread, input[thread]->count, input[thread]->runtime, input[thread]->min, input[thread]->max);
		total_time += input[thread]->runtime;
	}
	fprintf(stderr, "\nFang search took: %lf, average: %lf\n", total_time, total_time / NUM_THREADS);
#endif

#ifdef PRINT_VAMPIRE_COUNT
	fprintf(stderr, "Found: %llu vampire numbers.\n", result);
#endif

#if (defined PRINT_DISTRIBUTION_MATRIX) && (NUM_THREADS > 8)
	double distrubution = 0.0;
	for (thread = 0; thread < NUM_THREADS; thread++) {
		distrubution += ((double)input[thread]->count) / ((double)(result));
		fprintf(stderr, "(1+%u/%u,%lf),", thread+1, NUM_THREADS/9, distrubution);
		//printf("(1+%u/%u,%lf),", thread+1, NUM_THREADS/9, distrubution);
		//printf("(1+%u/%u,%llu/%llu),", thread, NUM_THREADS/9,input[thread]->count, result/(NUM_THREADS/9));
		if ((thread+1) % 10 == 0)
			fprintf(stderr, "\n");
	}
#endif
	for (thread = 0; thread<NUM_THREADS; thread++) {
		vargs_free(input[thread]);
	}
	pthread_exit(NULL);
	return 0;
}
