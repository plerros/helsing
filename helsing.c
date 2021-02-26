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

// Compile with: gcc -O3 -Wall -Wextra -pthread -o helsing helsing.c

/*--------------------------- COMPILATION OPTIONS  ---------------------------*/
#define NUM_THREADS 1 // Thread count above #cores may not improve performance
#define ITERATOR 1000000000000ULL // How long until new work is assigned to threads 1000000000000000ULL

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

unsigned short length (unsigned long long number)
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

unsigned long long pow10ull (unsigned short exponent) // Returns 10 ^ exponent
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

inline bool trailingzero (unsigned long long number){
	return (!(number % 10));
}

unsigned long long get_min (unsigned long long min, unsigned long long max){
	if (length_isodd(min)) {
		unsigned short min_length = length(min);
		if (min_length < length(max))
			min = pow10ull (min_length);
		else
			min = max;
	}
	return min;
}

unsigned long long get_max (unsigned long long min, unsigned long long max){
	if (length_isodd(max)){
		unsigned short max_length = length(max);
		if (max_length > length(min))
			max = pow10ull(max_length -1) -1;
		else
			max = min;
	}
	return max;
}

unsigned long long sqrtull ( unsigned long long s )
{
	unsigned long long x0 = s >> 1;				// Initial estimate

	// Sanity check
	if (x0){
		unsigned long long x1 = ( x0 + s / x0 ) >> 1;	// Update
		
		while ( x1 < x0 )				// This also checks for cycle
		{
			x0 = x1;
			x1 = ( x0 + s / x0 ) >> 1;
		}
		return x0;
	} else {
		return s;
	}
}

bool vampirism_check(unsigned long long product, unsigned long long multiplier, unsigned long long multiplicant)
{
	unsigned short product_array[10] = {0};
	unsigned short digit;
	while (product != 0){
		product_array[product % 10] += 1;
		product /= 10;
	}
	while (multiplier != 0){
		digit = multiplier % 10;
		if(product_array[digit] > 0)
			product_array[digit] -= 1;
		else
			return 0;
		multiplier /= 10;
	}
	while (multiplicant != 0){
		digit = multiplicant % 10;
		if(product_array[digit] > 0)
			product_array[digit] -= 1;
		else
			return 0;
		multiplicant /= 10;
	}
	for (int i = 0; i < 10; i++){
		if(product_array[i] != 0)
			return 0;
	}
	return 1;
}

/*---------------------------------- dllist ----------------------------------*/

/*
 *         |<------------- n <= 2^16 - 1 ------------->|
 *         [ digit 1 ]   [ digit 2 ]   ...   [ digit n ]
 * NULL <--[prev next]<->[prev next]<->...<->[prev next]--> NULL
*/

typedef struct dllist	/* Linked list of unsigned short digits*/
{
	unsigned short digit :4;
	struct dllist *prev;
	struct dllist *next;
} dllist;

dllist *dllist_init(unsigned short digit, dllist *prev, dllist *next)
{
	dllist *new = malloc(sizeof(dllist));
	assert(new != NULL);

	new->digit = digit;
	new->prev = prev;
	new->next = next;

	return new;
}


dllist *dllist_ull_init(unsigned long long number)	// N to 1 conversion
{
	dllist *new = NULL;
	dllist *temp = NULL;

	for(unsigned short i = length(number); i > 0; i--){
		new = dllist_init(number % 10, NULL, temp);
		if(temp != NULL)
			temp->prev = new;
		temp = new;
		number /= 10;
	}
	return new;
}

dllist *dllist_char_p_init(const char *str)	// 1 to N conversion
{
	dllist *new = NULL;
	dllist *current = NULL;
	dllist *temp = NULL;
	for (unsigned short i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
		current = dllist_init((str[i] - '0'), temp, NULL);
		if(new == NULL){
			new = current;
		} else {
			temp->next = current;
		}
		temp = current;
	}
	return new;
}

dllist *dllist_copy(dllist *original)	// 1 to N conversion
{
	dllist *copy = NULL;
	if (original != NULL) {
		dllist *current = NULL;
		dllist *temp = NULL;
		for(dllist *i = original; i != NULL; i = i->next){
			current = dllist_init(i->digit, temp, NULL);
			if(copy == NULL){
				copy = current;
			} else {
				temp->next = current;
			}
			temp = current;
		}	
	}
	return (copy);
}

int dllist_free(dllist *dllist_ptr)
{
	dllist *current = dllist_ptr;
	for (dllist *temp = current; current != NULL ;) {
		temp = current;
		current = current->next;
		free(temp);
	}
	return 0;
}

/*--------------------------------- dllhead  ---------------------------------*/

typedef struct dllhead
{
	struct dllist *first;
	struct dllist *last;
} dllhead;

dllhead *dllhead_init(dllist *first, dllist *last)
{
	dllhead *new = malloc(sizeof(dllhead));
	assert(new != NULL);
	new->first = first;

	for(new->last = first; new->last->next != NULL; new->last = new->last->next){}

	return new;
}

int dllhead_free(dllhead *dllhead_ptr)
{
	if (dllhead_ptr != NULL) {
		dllist_free(dllhead_ptr->first);
	}
	free(dllhead_ptr);
	return 0;
}

dllhead *dllhead_copy(dllhead *original) {
	dllist *first = NULL;
	dllist *last = NULL;
	first = dllist_copy(original->first);
	for(last = first; last->next != NULL; last = last->next){}
	dllhead *copy = dllhead_init(first,last);
	return (copy);
}

int dllhead_pop(dllhead *dllhead_ptr, unsigned short digit)
{
	dllist *prev = NULL;
	for (dllist *i = dllhead_ptr->first; i != NULL; i = i->next) {
		if (i->digit == digit) {
			if (i == dllhead_ptr->first) {
				dllhead_ptr->first = i->next;
			}
			if (i == dllhead_ptr->last) {
				dllhead_ptr->last = prev;
			}
			if (prev != NULL) {
				prev->next = i->next;
			}
			free(i);
			return 1;
		}
		else
			prev = i;
	}
	return 0;
}

int dllhead_increment(dllhead *dllhead_ptr)
{
	dllist *temp = dllhead_ptr->last;
	if(temp->digit == 9){
		for(;temp->digit == 9; temp = temp->prev){
			if(temp->prev == NULL){
				temp->prev = dllist_init(1, NULL, temp);
				dllhead_ptr->first = temp->prev;
			} else {
				temp->digit = 0;
			}
		}
	} else {
		temp->digit += 1;
	}
	return 0;
}

bool dllhead_vampirism_check(dllhead *product, dllhead *mutiplier, dllhead *multiplicant)
{
	unsigned short count[10] = {0};
	for(dllist *i = product->first; i != NULL; i = i->next){
		//printf("%d", i->digit);
		count[i->digit] += 1;
	}
	//printf("\n-");
	for(dllist *i = mutiplier->first; i != NULL; i = i->next){
		if(i->digit != 0 && count[i->digit] == 0){
			return false;
		}
		count[i->digit] -= 1;
		//printf("%d", i->digit);
	}
	//printf("\n-");
	for(dllist *i = multiplicant->first; i != NULL; i = i->next){
		if(count[i->digit] == 0){
			return false;
		}
		count[i->digit] -= 1;
		//printf("%d", i->digit);
	}
	//printf("\n");

	for(int i = 0; i < 10; i++){
		if(count[i] != 0){
			return false;
		}
	}
	return true;
}

int dllhead_print(dllhead *dllhead_ptr)
{

	printf("dllhead print:\t");
	for(dllist*i = dllhead_ptr->first; i != NULL; i = i->next){
		printf("%d", i->digit);
	}
	printf("\n");
	return 0;
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

int is_balanced(ullbtree *tree)
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
		//printf("Rotating right: %llu / %llu\n", left->value, tree->value);
		tree->left = left->right;
		ullbtree_reset_height(tree);
		left->right = tree;
		ullbtree_reset_height(left);
		//printf("Rotated right: %llu \\ %llu\n", left->value, tree->value);
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
		//printf("Rotating left: %llu \\ %llu\n", tree->value, right->value);
		tree->right = right->left;
		ullbtree_reset_height(tree);
		right->left = tree;
		ullbtree_reset_height(right);
		//printf("Rotated left: %llu / %llu\n", tree->value, right->value);
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
	unsigned long long step;
	unsigned long long count;
	double	runtime;
	double 	algorithm_runtime;
	ullbtree *result;
} vargs;

vargs *vargs_init(unsigned long long min, unsigned long long max, unsigned long long step)
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

int vargs_split(vargs *input[], unsigned long long min, unsigned long long max)
{
	unsigned long long current = 0;

	input[current]->min = min;
	input[current]->max = (max-min)/(NUM_THREADS - current) + min;

	for (; input[current]->max < max;) {
		min = input[current]->max + 1;
		current ++;
		input[current]->min = min;
		input[current]->max = (max-min)/(NUM_THREADS - current) + min;
	}

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
	unsigned long long factor_min = pow10ull((length(min) / 2) - 1);
	unsigned long long factor_max = pow10ull((length(max) / 2)) -1;

	unsigned long long product_max = factor_max * factor_max;

	unsigned long long max_sqrt;

	if(max >= product_max){
		max = product_max;
		max_sqrt = factor_max;
	} else {
		max_sqrt = max / sqrtull(max);
	}

	//Adjust range for Multiplier
	unsigned long long multiplier = factor_max;
	unsigned long long multiplier_min = min / sqrtull(min);

	//Adjust range for Multiplicant
	unsigned long long multiplicant_max;

	unsigned long long product;

	dllhead *dllhead_product = NULL;
	dllhead *dllhead_multiplier = NULL;
	dllhead *dllhead_multiplicant = NULL;

	printf("min max [%llu, %llu], fmin fmax [%llu, %llu], %llu\n", min, max, factor_min, factor_max, multiplier_min);

	for(unsigned long long multiplicant; multiplier >= multiplier_min; multiplier--){
		if(multiplier % 3 == 1){
			continue;
		}
		multiplicant = min/multiplier + !(!(min % multiplier));
		if(multiplier >= max_sqrt){
			multiplicant_max = max/multiplier;
		} else {
			multiplicant_max = multiplier -1; 
			//multiplier cannot be equal to mutiplicant, because a square number can end only with digits 0, 1, 4, 5, 6 or 9.
		}
		//if(multiplicant <= multiplicant_max && multiplicant <= multiplier_max){
			//printf("%llu, %llu, %llu = %llu\n", multiplier, multiplicant, multiplicant_max, multiplier * multiplicant);
		//}
		dllhead_multiplier = dllhead_init(dllist_ull_init(multiplier), NULL);
		dllhead_multiplicant = dllhead_init(dllist_ull_init(multiplicant-1), NULL);
		for(;multiplicant <= multiplicant_max; multiplicant++){
			//dllhead_print(dllhead_multiplicant);
			dllhead_increment(dllhead_multiplicant);
			//dllhead_print(dllhead_multiplicant);
			product = multiplier*multiplicant;
			//if(multiplicant > multiplier){
			//	printf("%llu, %llu, %llu = %llu\n", multiplier, multiplicant, multiplicant_max, multiplier * multiplicant);
			//}


			if((multiplier + multiplicant) % 9 != product % 9){
				//if((multiplier * (multiplicant + 9) == product + (multiplicant * 9))){
				//	printf("%llu = %llu * %llu\n", product, multiplier, multiplicant);
				//	assert(0);
				//}
				continue;
			}
			//if ((multiplicant + 2) % 3 == 0){
			//	continue;
			//}
			if (!(trailingzero(multiplier) && trailingzero(multiplicant))){
				//if ((product + 1) % 3 == 0){
				//	continue;
				//}
				//if (product <= max){
				
				dllhead_product = dllhead_init(dllist_ull_init(product), NULL);
					//bool testresult = vampirism_check(product, multiplier, multiplicant);
					bool testresult = dllhead_vampirism_check(dllhead_product, dllhead_multiplier,dllhead_multiplicant);
					if (testresult) {
						if (args->result == NULL) {
							args->result = ullbtree_init(product);
							args->count += 1;
						} else {
							args->result = ullbtree_add(args->result, ullbtree_init(product), &(args->count));
						}
					}
				
				dllhead_free(dllhead_product);
			}
		}
		dllhead_free(dllhead_multiplier);
		dllhead_free(dllhead_multiplicant);
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
		printf("Usage: vampire [min] [max]\n");
		return 0;
	}
	unsigned long long i;
	unsigned long long min = atoull(argv[1]);
	unsigned long long max = atoull(argv[2]);

	if (min > max) {
		printf("Usage: vampire [min] [max],\nwhere 0 <= min <= max and min <= max < %llu", ULLONG_MAX);
		return 0;
	}

	min = get_min(min, max);
	max = get_max(min, max);

	unsigned long long lmin = min;
	unsigned long long l_roof = pow10ull(length(lmin)) - 1;
	unsigned long long lmax = max;
	if( max > l_roof)
		lmax = l_roof;


	int rc;
	pthread_t threads[NUM_THREADS];
	vargs *input[NUM_THREADS];
	unsigned long long thread;
	for (i = 0; i < NUM_THREADS; i++) {
		input[i] = vargs_init(0, 0, 1);
	}
	ullbtree *result_tree = NULL;
	unsigned long long iterator = ITERATOR;
	unsigned long long active_threads = NUM_THREADS;


	for (; lmax <= max;) {
		printf("Checking range: [%llu, %llu]\n", lmin, lmax);
		for (i = lmin; i <= lmax; i += iterator + 1) {
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
						result_tree = ullbtree_add(result_tree, input[thread]->result);
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
			lmax = get_max (lmin, max);
			l_roof = pow10ull(length(lmin)) - 1;
			if(max > l_roof)
				lmax = l_roof;

			if (lmax-lmin > ITERATOR)
				iterator = ITERATOR;
			else
				iterator = lmin -lmax;
		} else {
			break;
		}
	}
	unsigned long long result = 0;

#ifdef SPD_TEST
	double algorithm_time = 0.0;
	printf("Thread  Count   Rutime\n");
#endif

	for (thread = 0; thread<NUM_THREADS; thread++) {
#ifdef SPD_TEST
		printf("%llu\t%llu\t%lf\t[%llu\t%llu]\n", thread, input[thread]->count, input[thread]->runtime, input[thread]->min, input[thread]->max);
		algorithm_time += input[thread]->algorithm_runtime,
#endif
		result += input[thread]->count;
		vargs_free(input[thread]);
	}
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
