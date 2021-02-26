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

// Compile with: gcc -O3 -Wall -Wextra -pthread -lm -o helsing helsing.c
// Record to break: The 208423682 14-digit vampires were computed to a 7 GB file in 19 hours on November 12-13 2002.

/*--------------------------- COMPILATION OPTIONS  ---------------------------*/
#define NUM_THREADS 16 // Thread count above #cores may not improve performance 
#define ITERATOR 100000000000000ULL // How long until new work is assigned to threads 1000000000000000ULL
//18446744073709551616

//#define SANITY_CHECK
//#define OEIS_OUTPUT

//#define MEASURE_RUNTIME
#define PRINT_VAMPIRE_COUNT
#define DISTRIBUTION_COMPENSATION

/*---------------------------- PREPROCESSOR_STUFF ----------------------------*/
#ifndef OEIS_OUTPUT

#ifdef PRINT_VAMPIRE_COUNT
#define RESULTS
#endif

#if defined(MEASURE_RUNTIME) && (defined(CLOCK_MONOTONIC) || defined(CLOCK_REALTIME))
#define SPD_TEST

#if defined(CLOCK_MONOTONIC)
#define SPDT_CLK_MODE CLOCK_MONOTONIC
#elif defined(CLOCK_REALTIME)
#define SPDT_CLK_MODE CLOCK_REALTIME
#endif

#endif

#endif

/*----------------------------- ULLONG FUNCTIONS -----------------------------*/

uint8_t length(unsigned long long number)
{
	if (number < 10000000000ULL) {
		if (number < 100000ULL) {
			if (number < 1000ULL) {
				if (number < 10ULL)
					return 1;
				else if (number < 100ULL)
					return 2;
				else
					return 3;
			} else {
				if (number < 10000ULL)
					return 4;
				else
					return 5;
			}
		} else {
			if (number < 100000000ULL) {
				if (number < 1000000ULL)
					return 6;
				else if (number < 10000000ULL)
					return 7;
				else
					return 8;
			} else {
				if (number < 1000000000ULL)
					return 9;
				else
					return 10;
			}
		}
	} else {
		if (number < 1000000000000000ULL) {
			if (number < 10000000000000ULL) {
				if (number < 100000000000ULL)
					return 11;
				else if (number < 1000000000000ULL)
					return 12;
				else
					return 13;
			} else {
				if (number < 100000000000000ULL)
					return 14;
				else
					return 15;
			}
		} else {
			if (number < 1000000000000000000ULL) {
				if (number < 10000000000000000ULL)
					return 16;
				else if (number < 100000000000000000ULL)
					return 17;
				else
					return 18;
			} else {
				if (number < 10000000000000000000ULL)
					return 19;
				else
					return 20;
			}
		}
	}
}

bool length_isodd(unsigned long long number)
{
	return (length(number) % 2);
}

unsigned long long pow10ull(uint8_t exponent) // Returns 10 ^ exponent
{
#ifdef SANITY_CHECK
	assert(exponent <= length(ULLONG_MAX) - 1);
#endif

	if (exponent < 10) {
		if (exponent < 5) {
			if (exponent < 3) {
				if (exponent == 0)
					return 1ULL;
				else if (exponent == 1)
					return 10ULL;
				else
					return 100ULL;
			} else {
				if (exponent == 3)
					return 1000ULL;
				else
					return 10000ULL;
			}
		} else {
			if (exponent < 8) {
				if (exponent == 5)
					return 100000ULL;
				else if (exponent == 6)
					return 1000000ULL;
				else
					return 10000000ULL;
			} else {
				if (exponent == 8)
					return 100000000ULL;
				else
					return 1000000000ULL;
			}
		}
	} else {
		if (exponent < 15) {
			if (exponent < 13) {
				if (exponent == 10)
					return 10000000000ULL;
				else if (exponent == 11)
					return 100000000000ULL;
				else
					return 1000000000000ULL;
			} else {
				if (exponent == 13)
					return 10000000000000ULL;
				else
					return 100000000000000ULL;
			}
		} else {
			if (exponent < 18) {
				if (exponent == 15)
					return 1000000000000000ULL;
				else if (exponent == 16)
					return 10000000000000000ULL;
				else
					return 100000000000000000ULL;
			} else {
				if (exponent == 18)
					return 1000000000000000000ULL;
				else
					return 10000000000000000000ULL;
			}
		}
	}
}

unsigned long long atoull(const char *str, bool *error)	// ASCII to unsigned long long
{
	unsigned long long number = 0;
	unsigned long long temp;
	for (uint8_t i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
		temp = 10 * number + (str[i] - '0');
		if (temp < number) {  // overflow
			*error = true;
			return 1;
		}
		number = temp;
	}
	return number;
}

bool notrailingzero(unsigned long long number)
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
			max = pow10ull(max_length -1) -1;
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

unsigned long long sqrtull(unsigned long long s)
{
	unsigned long long x0 = s >> 1;						// Initial estimate

	if (x0) {											// Sanity check
		unsigned long long x1 = (x0 + s / x0) >> 1;		// Update
		while (x1 < x0) {								// This also checks for cycle
			x0 = x1;
			x1 = (x0 + s / x0) >> 1;
		}
		return x0;
	} else {
		return s;
	}
}

/*--------------------------------- ULLBTREE ---------------------------------*/

typedef struct ullbtree
{
	struct ullbtree *left;
	struct ullbtree *right;
	unsigned long long value;
	unsigned long long height;
} ullbtree;

/*
 * Unlike other binary trees, if right and left are not present, their height isn't set to -1.
 * Instead, their memory location is NULL. This minimizes memory usage.
*/

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

/*
 * If the tree is within the balance limit it returns 0
 * If the left is bigger, it returns 1
 * If the right is bigger, it returns -1;
*/

int8_t is_balanced(ullbtree *tree)
{
	//assert(tree != NULL);
	if (tree != NULL) {
		if (tree->left != NULL && tree->right != NULL) {
			if (tree->left->height > tree->right->height) {	//We do this to avoid overflow
				if (tree->left->height > tree->right->height + 1)
					return 1;
				else
					return 2;
			}
			else if (tree->right->height > tree->left->height) {	//We do this to avoid overflow
				if (tree->right->height > tree->left->height + 1)
					return -1;
				else
					return -2;
			}
		}
		else if (tree->left != NULL) {
			if (tree->left->height > 0)
				return 1;
			else
				return 2;
		}
		else if (tree->right != NULL) {
			if (tree->right->height > 0)
				return -1;
			else
				return -2;
		}
	}
	return 0;
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

/*
 * Binary tree left rotation:
 * 
 *       A             B
 *      / \           / \
 *    ...  B   -->   A  ...
 *        / \       / \
 *       C  ...   ...  C
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

ullbtree *ullbtree_balance(ullbtree *tree)
{
	//assert(tree != NULL);
	switch(is_balanced(tree)){
		case 1:
			if (is_balanced(tree->left) < 0) {
				tree->left = ullbtree_rotate_l(tree->left);
				ullbtree_reset_height(tree); //maybe optional?
			}
			tree = ullbtree_rotate_r(tree);
			break;

		case -1:
			if (is_balanced(tree->right) > 0) {
				tree->right = ullbtree_rotate_r(tree->right);
				ullbtree_reset_height(tree); //maybe optional?
			}
			tree = ullbtree_rotate_l(tree);
			break;

		default:
			return tree;
	}
	return tree;
}

ullbtree *ullbtree_add(ullbtree *tree, unsigned long long node, unsigned long long *count)
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

/*----------------------------------------------------------------------------*/

typedef struct vargs	/* Vampire arguments */
{
	unsigned long long min;
	unsigned long long max;
	unsigned long long count;
	double	runtime;
	ullbtree *result;
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
	return new;
}

int vargs_free(vargs *vargs_ptr)
{
	ullbtree_free(vargs_ptr->result);
	free (vargs_ptr);
	return 0;
}

double distribution_inverted_integral(double area)
{
	//printf("%lf\n",  1.0 - 0.9 * pow(1.0-area, 1.0/3.0));
	return (1.0 - 0.9 * pow(1.0-area, 1.0/(3.0-(0.7*area))));//This is a hand made approximation.
}

uint16_t vargs_split(vargs *input[], unsigned long long min, unsigned long long max)
{
	uint16_t current = 0;
	input[current]->min = min;
	input[current]->max = (max-min)/(NUM_THREADS - current) + min;

	for (; input[current]->max < max;) {
		min = input[current]->max + 1;
		current ++;
		input[current]->min = min;
		input[current]->max = (max-min)/(NUM_THREADS - current) + min;
	}

#ifdef DISTRIBUTION_COMPENSATION
	//unsigned long long factor_min = pow10ull((length(min)) - 1);
	unsigned long long factor_max = pow10ull((length(max))) -1;
	unsigned long long temp;
	for (uint16_t i = 0; i < current; i++) {
		temp = (double)factor_max * distribution_inverted_integral(((double)i+1)/((double)NUM_THREADS));

		if (temp > input[i]->min && temp < input[i]->max) {
			//printf("min %llu, bl %llu, bli %llu, bli(bl) %llu\n", input[i]->max, bl_transform(input[i]->max),bl_invert(input[i]->max), bl_invert(bl_transform(input[i]->max)));
			input[i]->max = temp;
			input[i+1]->min = temp + 1;
		}
	}
#endif

	input[current]->max = max;
	return (current);
}

/*----------------------------------------------------------------------------*/

void *vampire(void *void_args)
{
#ifdef SPD_TEST
	struct timespec start, finish;
	double elapsed;
	clock_gettime(SPDT_CLK_MODE, &start);
#endif

	vargs *args = (vargs *)void_args;

	unsigned long long min = args->min;
	unsigned long long max = args->max;

	//Min Max range for both factors
	//unsigned long long factor_min = pow10ull((length(min) / 2) - 1);
	unsigned long long factor_max = pow10ull((length(max) / 2)) -1;

	if (max >= factor_max * factor_max)
		max = factor_max * factor_max;
	
	unsigned long long min_sqrt = min / sqrtull(min);
	unsigned long long max_sqrt = max / sqrtull(max);

	//Adjust range for Factors
	unsigned long long multiplier = min_sqrt;
	unsigned long long multiplicant_max;
	unsigned long long product_iterator;

	unsigned long long product;
	bool mult_zero;

	//printf("min max [%llu, %llu], fmin fmax [%llu, %llu], %llu\n", min, max, factor_min, factor_max, multiplier);
	for (unsigned long long multiplicant; multiplier <= factor_max; multiplier++) {
		if (multiplier % 3 == 1) {
			continue;
		}
		multiplicant = min/multiplier + !!(min % multiplier); // fmin * fmax <= min - 10^n

		if (multiplier >= max_sqrt) {
			multiplicant_max = max/multiplier;
		} else {
			multiplicant_max = multiplier; //multiplicant can be equal to multiplier, 5267275776 = 72576 * 72576.
		}

		if (multiplicant <= multiplicant_max) {
			uint8_t mult_array[10] = {0};
			for (unsigned long long i = multiplier; i != 0; i /= 10) {
				mult_array[i % 10] += 1;
			}

			mult_zero = notrailingzero(multiplier);

			/*
			 * Modulo 9 check:
			 * for (;multiplicant <= multiplicant_max && ((multiplier + multiplicant) % 9 != (multiplier*multiplicant) % 9); multiplicant ++) {}
			*/

			/*
			 * Modulo 9 check, slightly simplified:
			 * for (;multiplicant <= multiplicant_max && ((multiplier - 1) * multiplicant - multiplier) % 9 != 0; multiplicant ++) {}
			*/

			/*
			 * Modulo 9 check, simplified a bit further:
			*/
			unsigned long long A_1 = multiplier - 1;
			unsigned long long AB_A_B = A_1 * (multiplicant - 1) - 1;
			for(unsigned long long CA_1 = 0; (multiplicant <= multiplicant_max) && ((AB_A_B + CA_1) % 9 != 0); CA_1 += A_1){multiplicant++;}


			product_iterator = multiplier * 9;
			product = multiplier*(multiplicant);

			for (;multiplicant <= multiplicant_max; multiplicant += 9) {
				uint16_t product_array[10] = {0};
				for (unsigned long long p = product; p != 0; p /= 10) {
					product_array[p % 10] += 1;
				}
				for (uint8_t i = 0; i < 10; i++) { //Yes, we want to check all 10, this is faster than only checking 8.
					if (product_array[i] < mult_array[i]) {
						goto vampire_exit;
					}
				}
				uint8_t temp;
				for (unsigned long long m = multiplicant; m != 0; m /= 10) {
					temp = m % 10;
					if (product_array[temp] < 1) {
						goto vampire_exit;
					} else {
						product_array[temp]--;
					}
				}

				for (uint8_t i = 0; i < 8; i++) {
					if (product_array[i] != mult_array[i]) {
						goto vampire_exit;
					}
				}
	
				if ((mult_zero || notrailingzero(multiplicant))) {
					args->result = ullbtree_add(args->result, product, &(args->count));
				}
vampire_exit:
				product += product_iterator;
			}
		}
	}

#ifdef SPD_TEST
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
		printf("A vampire number generator\nUsage: vampire [min] [max]\n");
		return 0;
	}

	bool error = 0;
	unsigned long long min = atoull(argv[1], &error);
	if (error) {
		printf("Minimum value overflows the limit: %llu\n", ULLONG_MAX);
		return 1;
	}
	unsigned long long max = atoull(argv[2], &error);
	if (error) {
		printf("Maximum value overflows the limit: %llu\n", ULLONG_MAX);
		return 1;
	}

	if (min > max) {
		printf("Invalid arguments, min <= max\n");
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

#ifndef OEIS_OUTPUT
		printf("Checking range: [%llu, %llu]\n", lmin, lmax);
#endif
		iterator = ITERATOR;
		for (unsigned long long i = lmin; i <= lmax; i += iterator + 1) {
			if (lmax - i < ITERATOR)
				iterator = lmax - i;

			active_threads = vargs_split(input, i, i + iterator) + 1;

			for (thread = 0; thread < active_threads; thread++) {
				returncode = pthread_create(&threads[thread], NULL, vampire, (void *)input[thread]);
				if (returncode) {
					printf("Error: unable to create thread %d\n", returncode);
					return 1;
				}
			}
			for (thread = 0; thread < active_threads; thread++) {
				pthread_join(threads[thread], 0);
				result = ullbtree_results(input[thread]->result, result);
				ullbtree_free(input[thread]->result);
				input[thread]->result = NULL;
			}
		}
		if (lmax != max) {
			lmin = get_min (lmax + 1, max); // lmax + 1 <= ULLONG_MAX, because lmax < max.
			lmax = get_lmax(lmin, max);
		} else {
			break;
		}
	}

#ifdef SPD_TEST
	double total_time = 0.0;
	printf("Thread  Count   Runtime\n");
	for (thread = 0; thread<NUM_THREADS; thread++) {
		printf("%u\t%llu\t%lf\t[%llu\t%llu]\n", thread, input[thread]->count, input[thread]->runtime, input[thread]->min, input[thread]->max);
		total_time += input[thread]->runtime;
	}
	printf("\nFang search took: %lf, average: %lf\n", total_time, total_time / NUM_THREADS);
#endif

#ifdef RESULTS
	printf("Found: %llu vampire numbers.\n", result);
#endif

#if (defined SPD_TEST) && (NUM_THREADS > 8)
	double distrubution = 0.0;
	for (thread = 0; thread < NUM_THREADS; thread++) {
		distrubution += ((double)input[thread]->count) / ((double)(result));
		printf("(1+%u/%u,%lf),", thread+1, NUM_THREADS/9, distrubution);
		//printf("(1+%u/%u,%llu/%llu),", thread, NUM_THREADS/9,input[thread]->count, result/(NUM_THREADS/9));
		if ((thread+1) % 10 == 0)
			printf("\n");
	}
#endif
	//printf("\n");
	for (thread = 0; thread<NUM_THREADS; thread++) {
		vargs_free(input[thread]);
	}
	pthread_exit(NULL);
	return 0;
}
