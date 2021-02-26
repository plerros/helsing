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

// Compile with: gcc -O3 -Wall -Wextra -pthread -o helsing helsing.c -lm
//Record to break: The 208423682 14-digit vampires were computed to a 7 GB file in 19 hours on November 12-13 2002.

/*--------------------------- COMPILATION OPTIONS  ---------------------------*/
#define NUM_THREADS 9 // Thread count above #cores may not improve performance 
#define ITERATOR 100000000000000ULL // How long until new work is assigned to threads 1000000000000000ULL
//18446744073709551616

//#define SANITY_CHECK
//#define OEIS_OUTPUT

#define MEASURE_RUNTIME
#define PRINT_VAMPIRE_COUNT

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

unsigned long long atoull (const char *str)	// ASCII to unsigned long long
{
	unsigned long long number = 0;
	for (unsigned short i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
		number = 10 * number + (str[i] - '0');
	}
	return number;
}

uint8_t length (unsigned long long number)
{
	if (number < 10000000000ULL){
		if (number < 100000ULL) {
			if (number < 1000ULL){
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
			if (number < 1000000000000000000ULL){
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

bool length_isodd (unsigned long long number)
{
	number = number / 10;
	bool result = true;
	unsigned long long i = 1;
	while (i < number) {
		i *= 10;
		result = !result;
	}
	if (i == number) {
		result = !result;
	}
	return result;
}

unsigned long long pow10ull (uint8_t exponent) // Returns 10 ^ exponent
{
#ifdef SANITY_CHECK
	assert(exponent <= length(ULLONG_MAX));
#endif

	if (exponent < 10){
		if (exponent < 5) {
			if (exponent < 3){
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
			if (exponent < 18){
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
					return 1000000000000000000ULL;
			}
		}
	}
}

inline bool trailingzero (unsigned long long number)
{
	return ((number % 10) != 0);
}

unsigned long long get_min ( unsigned long long min, unsigned long long max )
{
	if (length_isodd(min)) {
		uint8_t min_length = length(min);
		if (min_length < length(max))
			min = pow10ull (min_length);
		else
			min = max;
	}
	return min;
}

unsigned long long get_max ( unsigned long long min, unsigned long long max )
{
	if (length_isodd(max)){
		uint8_t max_length = length(max);
		if (max_length > length(min))
			max = pow10ull(max_length -1) -1;
		else
			max = min;
	}
	return max;
}

unsigned long long get_lmax ( unsigned long long lmin, unsigned long long max )
{
	unsigned long long lmax;
	if(length(lmin) < length(ULLONG_MAX)){
		lmax = pow10ull(length(lmin)) - 1;
		if( lmax < max )
			return lmax;
	}
	return max;
}

unsigned long long sqrtull ( unsigned long long s )
{
	unsigned long long x0 = s >> 1;						// Initial estimate

	if (x0){											// Sanity check
		unsigned long long x1 = ( x0 + s / x0 ) >> 1;	// Update
		while (x1 < x0){								// This also checks for cycle
			x0 = x1;
			x1 = ( x0 + s / x0 ) >> 1;
		}
		return x0;
	} else {
		return s;
	}
}

bool vampirism_check(unsigned long long product, unsigned long long multiplicant, uint8_t mult_array[])
{
	int8_t product_array[10] = {0};

	for (; multiplicant != 0; multiplicant /= 10){
		product_array[multiplicant % 10] -= 1;		//Underflow is expected here.
	}
	for (; product != 0; product /= 10){
		product_array[product % 10] += 1;
	}
	for (uint8_t i = 0; i < 8; i++){ //maybe just 8?
		if(product_array[i] != mult_array[i])
			return false;
	}
	return true;
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
	if(tree != NULL){
		if (tree->left != NULL)
			ullbtree_free(tree->left);
		if (tree->right != NULL)
			ullbtree_free(tree->right);
	}
	free(tree);
	return 0;
}

int print_tree(ullbtree *tree, unsigned long long i)
{
	if(tree !=NULL){
		i = print_tree(tree->left, i);
		printf("%llu %llu\n", i, tree->value);
		i++;
		i = print_tree(tree->right, i);
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
	if(tree != NULL){
		if(tree->left != NULL && tree->right != NULL){
			if(tree->left->height > tree->right->height){	//We do this to avoid overflow
				if(tree->left->height > tree->right->height + 1)
					return 1;
				else
					return 2;
			}
			else if(tree->right->height > tree->left->height){	//We do this to avoid overflow
				if(tree->right->height > tree->left->height + 1)
					return -1;
				else
					return -2;
			}
		}
		else if(tree->left != NULL){
			if(tree->left->height > 0)
				return 1;
			else
				return 2;
		}
		else if(tree->right != NULL){
			if(tree->right->height > 0)
				return -1;
			else
				return -2;
		}
	}
	return 0;
}

int ullbtree_reset_height(ullbtree *tree)
{
	//assert(tree != NULL);
	tree->height = 0;
		if(tree->left != NULL && tree->left->height >= tree->height)
			tree->height = tree->left->height + 1;
		if(tree->right != NULL && tree->right->height >= tree->height)
			tree->height = tree->right->height + 1;
		return 0;
}

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
			if(is_balanced(tree->left) < 0){
				tree->left = ullbtree_rotate_l(tree->left);
				ullbtree_reset_height(tree); //maybe optional?
			}
			tree = ullbtree_rotate_r(tree);
			break;

		case -1:
			if(is_balanced(tree->right) > 0){
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

ullbtree *ullbtree_add(ullbtree *tree, ullbtree *node, unsigned long long *count)
{
	assert(tree != NULL);
	if(node->value == tree->value){
		ullbtree_free(node);
		return tree;
	}
	else if(node->value < tree->value){
		if(tree->left == NULL){
			tree->left = node;
			if(count != NULL){
				*count += 1;
			}
		} else {
			tree->left = ullbtree_add(tree->left, node, count);
		}
	} else {
		if(tree->right == NULL){
			tree->right = node;
			if(count != NULL){
				*count += 1;
			}
		} else {
			tree->right = ullbtree_add(tree->right, node, count);
		}
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
	uint16_t step;	//step is usually 1. Alternative thread load balancing changes this to #threads.
	unsigned long long count;
	double	runtime;
	double 	algorithm_runtime;
	ullbtree *result;
} vargs;

vargs *vargs_init(unsigned long long min, unsigned long long max, uint16_t step)
{
	vargs *new = malloc(sizeof(vargs));
	assert(new != NULL);
	new->min = min;
	new->max = max;
	new->step = step;
	new->count = 0;
	new->runtime = 0.0;
	new->algorithm_runtime = 0.0;
	new->result = NULL;
	return new;
}

int vargs_free(vargs *vargs_ptr)
{
	ullbtree_free(vargs_ptr->result);
	free (vargs_ptr);
	return 0;
}

inline double benfords_law(double digit)
{
	return (log10(1.0+1.0/digit));
}

inline double inverse_benfords_law(double var)
{
	return (1.0/(pow(10.0, var) - 1.0));
}

double benfords_law_sum(uint8_t n){
	double result = 0;
	if(n > 0){
		for(uint8_t i = 1; i <= n; i++){
			result += benfords_law(i);
		}
	}
	return result;
}

double inverse_benfords_law_sum(uint8_t n){
	double result = 0;
	if(n > 0){
		for(uint8_t i = 1; i <= n; i++){
			result += inverse_benfords_law(i);
		}
	}
	return result;
}

unsigned long long bl_transform (unsigned long long number)
{
	unsigned long long min = pow10ull(length(number)-1);
	unsigned long long max = pow10ull(length(number))-1;
	uint8_t n = 0;

	if(number >= min && number < 2 * min){
		n = 1;
	}
	else if(number >= 2 * min && number < 3 * min){
		n = 2;
	}
	else if(number >= 3 * min && number < 4 * min){
		n = 3;
	}
	else if(number >= 4 * min && number < 5 * min){
		n = 4;
	}
	else if(number >= 5 * min && number < 6 * min){
		n = 5;
	}
	else if(number >= 6 * min && number < 7 * min){
		n = 6;
	}
	else if(number >= 7 * min && number < 8 * min){
		n = 7;
	}
	else if(number >= 8 * min && number < 9 * min){
		n = 8;
	}
	else{
		n = 9;
	}
/*
	unsigned long long result = 
		max * (
			(benfords_law(n)-benfords_law(n+1))
			*((double)n- (double)number/(double)min)
			+benfords_law_sum(n)
		);
*/
	//printf("num %llu, ratio %lf, bl %lf, %d\n", number, (double)number / (double)min - (double)(n-1), benfords_law(n), n);
	//printf("%lf\n", benfords_law(n)*((double)number / (double)min ) +benfords_law_sum(n-1));

	unsigned long long result;

	if(n == 1){
		result = 
			max * (
				/*
				((benfords_law(n) - 0.1)/0.9)
				*(
					(double)number / (double)max - 0.1
				)
				+0.1
				*/
				(
					(benfords_law(n) - 0.1) / 0.1
				)*(
					(double)number/(double)max
				) + 0.2 - benfords_law(n)
			);
	} else {
		result =
			max * (
				benfords_law(n)
				*(
					(
						(double)number/(double)max
					)*10.0 - (double)n
				)
				+ benfords_law_sum(n-1)
			);
	}


	return result;
}

unsigned long long bl_invert (unsigned long long number)
{
	//unsigned long long min = pow10ull(length(number)-1);
	unsigned long long max = pow10ull(length(number))-1;
	//printf("%llu, %llu\n", min, max);
	uint8_t n = 0;

	if(number < max * benfords_law_sum(1)){
		n = 1;
	}
	else if(number >= max * benfords_law_sum(1) && number < max * benfords_law_sum(2)){
		n = 2;
	}
	else if(number >= max * benfords_law_sum(2) && number < max * benfords_law_sum(3)){
		n = 3;
	}
	else if(number >= max * benfords_law_sum(3) && number < max * benfords_law_sum(4)){
		n = 4;
	}
	else if(number >= max * benfords_law_sum(4) && number < max * benfords_law_sum(5)){
		n = 5;
	}
	else if(number >= max * benfords_law_sum(5) && number < max * benfords_law_sum(6)){
		n = 6;
	}
	else if(number >= max * benfords_law_sum(6) && number < max * benfords_law_sum(7)){
		n = 7;
	}
	else if(number >= max * benfords_law_sum(7) && number < max * benfords_law_sum(8)){
		n = 8;
	}
	else {
		n = 9;
	}
	//for(int i = 1; i < 10; i++)
		//printf("%llu\n",(unsigned long long)(max*benfords_law_sum(i)));

	//printf("num %llu, ratio %lf, sum %lf, bl %lf, %d\n", number, ((double)number/(double)max) - benfords_law_sum(n-1), benfords_law_sum(n-1), benfords_law(n), n);
	//printf("%lf\n", ((double)number/(double)max -0.2 + benfords_law(n))*0.1 +0.1     );
	unsigned long long result;
	if(n == 1){
		result = 
			max * (
				/*
				((double)number/(double)max - 0.1)
				/(
					(benfords_law(n) - 0.1)/ 0.9
				)
				+0.1
				*/
				(
					(
						(double)number/(double)max 
						-0.2
						+benfords_law(n)
					)
					/(benfords_law(n)-0.1)
				)*0.1 
			);
	} else {
		result =
			max * (
				(
					(
						((double)number/(double)max - benfords_law_sum(n-1))
						/benfords_law(n)
					)
					+(double)n
				)
				/10.0
			);
	}
	//printf("result %lld\n", result);
	return result;
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
/*
	unsigned long long temp;
	for(uint16_t i = 0; i < current; i++){
		temp = bl_invert(input[i]->max);
		//printf("min %llu, bl %llu, bli %llu, bli(bl) %llu\n", input[i]->max, bl_transform(input[i]->max),bl_invert(input[i]->max), bl_invert(bl_transform(input[i]->max)));

		if(temp > input[i]->min && temp < input[i]->max){
			printf("min %llu, bl %llu, bli %llu, bli(bl) %llu\n", input[i]->max, bl_transform(input[i]->max),bl_invert(input[i]->max), bl_invert(bl_transform(input[i]->max)));
			input[i]->max = temp;
			input[i+1]->min = temp + 1;
		}
	}

*/
	input[current]->max = max;
	return (current);
}

/*
int vargs_split(vargs *input[], unsigned long long min, unsigned long long max)
{
	int current = 0;

	input[current]->min = min;
	input[current]->max = max;

	for (; current < NUM_THREADS && min + current < max; current++){
		input[current]->min = min + current;
		input[current]->max = max;
	}
	return (current-1);
}
*/

/*----------------------------------------------------------------------------*/

void *vampire(void *void_args)
{
#ifdef SPD_TEST
	struct timespec start, finish;
	struct timespec fang_start, fang_finish;
	double elapsed;
	double fang_time;
	clock_gettime(SPDT_CLK_MODE, &start);
#endif

#ifdef SPD_TEST
	clock_gettime(SPDT_CLK_MODE, &fang_start);
#endif

	vargs *args = (vargs *)void_args;

	unsigned long long min = args->min;
	unsigned long long max = args->max;

	//Min Max range for both factors
	//unsigned long long factor_min = pow10ull((length(min) / 2) - 1);
	unsigned long long factor_max = pow10ull((length(max) / 2)) -1;

	if(max >= factor_max * factor_max)
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
	for(unsigned long long multiplicant; multiplier <= factor_max; multiplier++){
		if(multiplier % 3 == 1){
			continue;
		}
		multiplicant = min/multiplier + !(!(min % multiplier)); // fmin * fmax <= min - 10^n
		if(multiplier >= max_sqrt){
			multiplicant_max = max/multiplier;
		} else {
			multiplicant_max = multiplier; //multiplicant can be equal to mutiplicant, 5267275776 = 72576 * 72576.
		}
		uint8_t mult_array[10] = {0};
		for(unsigned long long i = multiplier; i != 0; i /= 10){
			mult_array[i % 10] += 1;
		}

		mult_zero = trailingzero(multiplier);
		for(;multiplicant <= multiplicant_max && ((multiplier + multiplicant) % 9 != (multiplier*multiplicant) % 9); multiplicant ++){}

		if(multiplicant <= multiplicant_max){
			product_iterator = multiplier * 9;
			product = multiplier*(multiplicant);
		}
		for(;multiplicant <= multiplicant_max; multiplicant += 9){
			if ((mult_zero || trailingzero(multiplicant)) && vampirism_check(product, multiplicant, mult_array)){
				if (args->result == NULL) {
					args->result = ullbtree_init(product);
					args->count += 1;
				} else {
					args->result = ullbtree_add(args->result, ullbtree_init(product), &(args->count));
				}
			}
			product += product_iterator;
		}
	}

#ifdef SPD_TEST
	clock_gettime(SPDT_CLK_MODE, &fang_finish);

	fang_time = (fang_finish.tv_sec - fang_start.tv_sec);
	fang_time += (fang_finish.tv_nsec - fang_start.tv_nsec) / 1000000000.0;
	args->algorithm_runtime += fang_time;
#endif

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
		printf("A posix compatible vampire number generator\n");
		printf("Usage: vampire [min] [max]\n");
		return 0;
	}

	unsigned long long min = atoull(argv[1]);
	unsigned long long max = atoull(argv[2]);

	if (min > max) {
		printf("Invalid arguments min <= max\n");
		return 0;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	unsigned long long lmin = min;	// local min for use in loop
	unsigned long long lmax = max;	// local max for use in loop
	unsigned long long l_roof;


/*
	l_roof = pow10ull(length(lmin)) - 1;
	if( max > l_roof)
		lmax = l_roof;
*/
	lmax = get_lmax(lmin, max);

	vargs *input[NUM_THREADS];
	uint16_t thread;

	for (thread = 0; thread < NUM_THREADS; thread++) {
		input[thread] = vargs_init(0, 0, 1);
	}

	int rc;
	pthread_t threads[NUM_THREADS];
	ullbtree *result_tree = NULL;
	unsigned long long iterator = ITERATOR;
	uint16_t active_threads = NUM_THREADS;

	for (; lmax <= max;) {
		printf("Checking range: [%llu, %llu]\n", lmin, lmax);
		for (unsigned long long i = lmin; i <= lmax; i += iterator + 1) {
			if (lmax-i < iterator) {
				iterator = lmax-i;
			}
			active_threads = vargs_split(input, i, i + iterator) + 1;

			for (thread = 0; thread < active_threads; thread++) {
				rc = pthread_create(&threads[thread], NULL, vampire, (void *)input[thread]);
				if (rc) {
					printf("Error:unable to create thread, %d\n", rc);
					exit(-1);
				}
			}
			for (thread = 0; thread < active_threads; thread++) {
				pthread_join(threads[thread], 0);
#if defined(OEIS_OUTPUT)
				if (input[thread]->result != NULL){
					if(result_tree != NULL){
						result_tree = ullbtree_add(result_tree, input[thread]->result, NULL);
					} else {
						result_tree = input[thread]->result;
					}
				}
				input[thread]->result = NULL;
#endif
			}
		}
		if(lmax < max){
			lmin = get_min (lmax + 1, max);
/*
			lmax = get_max (lmin, max);
			l_roof = pow10ull(length(lmin)) - 1;
			if(max > l_roof)
				lmax = l_roof;
*/
			lmax = get_lmax(lmin, max);
			if (lmax - lmin > ITERATOR)
				iterator = ITERATOR;
			else
				iterator = lmin - lmax;
		} else {
			break;
		}
	}
	unsigned long long result = 0;

#ifdef SPD_TEST
	double algorithm_time = 0.0;
	printf("Thread  Count   Runtime\n");
#endif

	for (thread = 0; thread<NUM_THREADS; thread++) {
#ifdef SPD_TEST
		printf("%u\t%llu\t%lf\t[%llu\t%llu]\n", thread, input[thread]->count, input[thread]->runtime, input[thread]->min, input[thread]->max);

		algorithm_time += input[thread]->algorithm_runtime;
#endif
		result += input[thread]->count;
		vargs_free(input[thread]);
	}
	for (thread = 0; thread < NUM_THREADS; thread++) {
#if (defined SPD_TEST) && (NUM_THREADS > 8)
		printf("(1+%u/%u,%llu/%llu),", thread, NUM_THREADS/9,input[thread]->count, result/(NUM_THREADS/9));
#endif
	}
	printf("\n");

#ifdef SPD_TEST
	printf("Fang search took: %lf, average: %lf\n", algorithm_time, algorithm_time / NUM_THREADS);
#endif

#if defined(OEIS_OUTPUT)
	print_tree(result_tree, 1);
#endif

#if defined(RESULTS)
	printf("Found: %llu vampire numbers.\n", result);
#endif

	ullbtree_free(result_tree);
	pthread_exit(NULL);
	return 0;
}
