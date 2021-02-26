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
	if ( x0 )
	{
		unsigned long long x1 = ( x0 + s / x0 ) >> 1;	// Update
		
		while ( x1 < x0 )				// This also checks for cycle
		{
			x0 = x1;
			x1 = ( x0 + s / x0 ) >> 1;
		}
		
		return x0;
	}
	else
	{
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
/*---------------------------------- ULIST  ----------------------------------*/

typedef struct ulist	// List of unsigned long long
{
	unsigned long long *array;	// Set to NULL when the array is empty. Check if NULL before traversing the array!
	unsigned short last;      	// Code should traverse the array up to [last]. May be smaller or equal than the actual size of the array.
} ulist;

ulist *ulist_init(unsigned long long number)
{
	ulist *new = malloc(sizeof(ulist));
	assert(new != NULL);

	new->last = length(number) - 1;

	new->array = malloc(sizeof(unsigned long long) * (new->last + 1));
	assert(new->array != NULL);

	for (unsigned long long i = new->last; i > 0; i--) {
		new->array[i] = number % 10;
		number = number/10;
	}
	new->array[0] = number % 10;
	return (new);
}

int ulist_free(ulist* ulist_ptr)
{
#ifdef SANITY_CHECK
	assert(ulist_ptr != NULL);
#endif
	if (ulist_ptr->array != NULL) {
		free(ulist_ptr->array);
		ulist_ptr->array = NULL;
	}
	free(ulist_ptr);
	return 0;
}

ulist *ulist_copy(ulist *original)
{
#ifdef SANITY_CHECK
	assert(original != NULL);
#endif

	ulist *copy = malloc(sizeof(ulist));
	assert(copy != NULL);

	copy->last = original->last;

	if (original->array != NULL) {
		copy->array = malloc(sizeof(unsigned long long)*(copy->last + 1));
		assert(copy->array != NULL);

		for (unsigned long long i = 0; i<=copy->last; i++) {
			copy->array[i] = original->array[i];
		}
	} else
		copy->array = NULL;

	return (copy);
}

int ulist_remove(ulist* ulist_ptr, unsigned long long element)
{
#ifdef SANITY_CHECK
	assert(ulist_ptr != NULL);
	assert(element <= ulist_ptr->last);
#endif

	if (ulist_ptr->array != NULL) {
		if (ulist_ptr->last == 0) {
			free(ulist_ptr->array);
			ulist_ptr->array = NULL;
		} else {
			for (; element < ulist_ptr->last; element++) {
				ulist_ptr->array[element] = ulist_ptr->array[element + 1];
			}
			ulist_ptr->last = ulist_ptr->last - 1;
		}
	}
	return 0;
}

int ulist_pop(ulist* ulist_ptr, unsigned long long number)
{
#ifdef SANITY_CHECK
	assert(ulist_ptr != NULL);
#endif

	if (ulist_ptr->array != NULL) {
		for (unsigned long long i = 0; i <= ulist_ptr->last; i++) {
			if (ulist_ptr->array[i] == number) {
				if (ulist_ptr->last == 0) {
					free(ulist_ptr->array);
					ulist_ptr->array = NULL;
				} else {
					for (; i < ulist_ptr->last; i++) {
						ulist_ptr->array[i] = ulist_ptr->array[i+1];
					}
					ulist_ptr->last = ulist_ptr->last - 1;
				}
				return 1;
			}
		}
	}
	return 0;
}

unsigned long long ulist_combine_digits(ulist *digits)
{
#ifdef SANITY_CHECK
	assert(digits != NULL);
#endif

	unsigned long long number = 0;
	if (digits->array != NULL) {
		for (unsigned long long i = 0; i <= digits->last; i++) {
			number = (number * 10)+ digits->array[i];
		}
	}
	return (number);
}

bool ulist_lastnotzero(ulist *ulist_ptr)
{
#ifdef SANITY_CHECK
	assert(ulist_ptr != NULL);
#endif

	if (ulist_ptr->array != NULL) {
		return (ulist_ptr->array[ulist_ptr->last] != 0);
	}
	return 0;
}

/*---------------------------------- LLIST  ----------------------------------*/

typedef struct llist	/* Linked list of unsigned long longegers */
{
	unsigned long long number;
	struct llist *next;
} llist;

llist *llist_init(unsigned long long number, llist* next)
{
	llist *new = malloc(sizeof(llist));
	assert(new != NULL);
	new->number = number;
	new->next = next;
	return (new);
}

int llist_free(llist *llist_ptr)
{
	for (llist *temp = NULL; llist_ptr != NULL ;) {
		temp = llist_ptr;
		llist_ptr = llist_ptr->next;
		free(temp);
	}
	return 0;
}

llist *get_fangs(unsigned long long dividend)
{
	llist *divisors = NULL;
	unsigned short dividend_length = length(dividend);
	unsigned short fang_length = dividend_length / 2;
	unsigned long long divisor = pow10ull(fang_length - 1);
	for (; length(dividend/divisor) > fang_length; divisor++) {}

	for (llist *current = NULL; divisor * divisor <= dividend; divisor++) {
		if (dividend % divisor == 0 && !( trailingzero(divisor) && trailingzero(dividend/divisor))) {
			if (current == NULL) {
				divisors = llist_init(divisor, NULL);
				current = divisors;
			} else {
				current->next = llist_init(divisor, current->next);
				current = current->next;
			}
		}
	}
	return (divisors);
}

llist *llist_copy(llist *original)
{
	llist *copy = NULL;
	if (original != NULL) {
		copy = llist_init(original->number, NULL);
		llist *current = copy;
		for (llist *i = original->next; i != NULL; i = i->next) {
			llist *llist_ptr = llist_init(i->number, NULL);
			current->next = llist_ptr;
			current = current->next;
		}
	}
	return (copy);
}

/*---------------------------------- LLHEAD ----------------------------------*/

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
	if (llhead_ptr != NULL) {
		llist_free(llhead_ptr->first);
	}
	free(llhead_ptr);
	return 0;
}

llhead *llhead_copy(llhead *original) {
	llist *first = NULL;
	first = llist_copy(original->first);
	llist *last = NULL;	/* this works but technically it shouldn't be just NULL. */
	llhead *copy = llhead_init(first,last);
	return (copy);
}

int llhead_pop(llhead *llhead_ptr, unsigned long long number)
{
	llist *prev = NULL;
	for (llist *i = llhead_ptr->first; i != NULL; i = i->next) {
		if (i->number == number) {
			if (i == llhead_ptr->first) {
				llhead_ptr->first = i->next;
			}
			if (i == llhead_ptr->last) {
				llhead_ptr->last = prev;
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
	unsigned long long fgh = max / max_sqrt;

	unsigned long long product;

	printf("min max [%llu, %llu], fmin fmax [%llu, %llu], %llu\n", min, max, factor_min, factor_max, multiplier_min);

	for(unsigned long long multiplicant; multiplier >= multiplier_min; multiplier--){
		if(multiplier % 3 == 1){
			continue;
		}
		multiplicant = min/multiplier + !(!(min % multiplier));
		if(multiplier >= fgh){
			multiplicant_max = max/multiplier;
		} else {
			multiplicant_max = multiplier;
		}
		//if(multiplicant <= multiplicant_max && multiplicant <= multiplier_max){
			//printf("%llu, %llu, %llu = %llu\n", multiplier, multiplicant, multiplicant_max, multiplier * multiplicant);
		//}
	
		for(;multiplicant <= multiplicant_max; multiplicant++){
			product = multiplier*multiplicant;
			//if(multiplicant > multiplier){
			//	printf("%llu, %llu, %llu = %llu\n", multiplier, multiplicant, multiplicant_max, multiplier * multiplicant);
			//}
			if((multiplier + multiplicant) % 9 != product % 9){
				continue;
			}
			//if ((multiplicant + 2) % 3 == 0){
			//	continue;
			//}
			if (!(trailingzero(multiplier) && trailingzero(multiplicant))){
				//if ((product + 1) % 3 == 0){
				//	continue;
				//}
				if (product <= max){
					bool testresult = vampirism_check(product, multiplier, multiplicant);
					if (testresult) {
						if (args->result == NULL) {
							args->result = ullbtree_init(product);
							args->count += 1;
						} else {
							args->result = ullbtree_add(args->result, ullbtree_init(product), &(args->count));
						}
					}
				}
			}
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
