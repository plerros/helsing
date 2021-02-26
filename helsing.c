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
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

/*
Compile with:
gcc -pthread -O3 -Wall -Wextra -ansi -o helsing helsing.c
*/

#define NUM_THREADS 8
#define ITERATOR 1000000 /* How long until new work is assigned to threads */

/*
#define __SANITY_CHECK__
#define __OEIS_OUTPUT__
*/

#define __SPEED_TEST__
#define __RESULT__


/*---------------------------- PREPROCESSOR_STUFF ----------------------------*/

#ifndef __OEIS_OUTPUT__

#ifdef __RESULT__
#define __RESULT___
#endif

#if defined(__SPEED_TEST__) && (defined(CLOCK_MONOTONIC) || defined(CLOCK_REALTIME))
#define __SPEED_TEST___

#if defined(CLOCK_MONOTONIC)
#define __CLOCK_MODE__ CLOCK_MONOTONIC
#elif defined(CLOCK_REALTIME)
#define __CLOCK_MODE__ CLOCK_REALTIME
#endif

#endif

#endif

/*----------------------------------------------------------------------------*/

unsigned long long atoull (const char * str)
{
	unsigned long long i;
	unsigned long long number = 0;
	for(i = 0; str[i] >= '0' && str[i] <= '9'; i++){
		number = 10 * number + (str[i] - '0');
	}

	return (number);
}

unsigned long long u_length (unsigned long long number)
{
	unsigned long long length = 1;
	for (; number > 9; length++) {
		number = number / 10;	/* Using division to avoid overflow for numbers around 2^64 -1 */
	}
	return (length);
}

unsigned long long u_length_isodd (unsigned long long number)
{
	unsigned long long result = 1;
	while (number > 9) {
		number = number / 10;	/* Using division to avoid overflow for numbers around 2^64 -1 */
		result = !result;
	}
	return (result);
}

unsigned long long ten_pow (unsigned long long exponent)
{
	unsigned long long result = 1;
	for(; exponent > 0; exponent --){
		result *= 10;
	}
	return (result);
}

unsigned long long u_sqrt(unsigned long long number)
{
	unsigned long long square_root = 1;
	while(square_root * square_root < number){
		square_root ++;
	}
	return (square_root);
}

unsigned long long u_sqrt_binary(unsigned long long number)
{
	unsigned long long square_root = number / 2;
	if(square_root > 4294967295)
		square_root = 4294967295;
	unsigned long long lim_min = 0;
	unsigned long long lim_max = number / 2;
	while(1){
		if((square_root * square_root <= number) && ((square_root + 1) * (square_root + 1) > number))
			return (square_root);
		
		if(square_root * square_root > number){
			lim_max = square_root;
			square_root = ((square_root - lim_min) / 2) + lim_min;
		}
		else{
			lim_min = square_root;
			square_root = ((lim_max - square_root) / 2) + square_root;
		}
	}
	return (0);
}

/*---------------------------------- ULIST  ----------------------------------*/

typedef struct ulist	/* List of unsigned long longeges */
{
	unsigned long long *array;	/* Set to NULL when the array is empty. Check if NULL before traversing the array! */
	unsigned long long last;	/* Code should traverse the array up to [last]. May be smaller or equal than the actual size of the array. */
} ulist;

ulist *ulist_init(unsigned long long number)
{
	ulist *new = malloc(sizeof(ulist));
	assert(new != NULL);

	new->last = u_length(number) - 1;

	new->array = malloc(sizeof(unsigned long long)*(new->last + 1));
	assert(new->array != NULL);

	unsigned long long i;
	for (i = new->last; i > 0; i--) {
		new->array[i] = number % 10;
		number = number/10;
	}
	new->array[0] = number % 10;
	return (new);
}

int ulist_free(ulist* ulist_ptr)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
#endif

	if (ulist_ptr->array != NULL) {
		free(ulist_ptr->array);
		ulist_ptr->array = NULL;
	}
	free(ulist_ptr);
	return (0);
}

ulist *ulist_copy(ulist *original)
{
#ifdef __SANITY_CHECK__
	assert(original != NULL);
#endif

	ulist *copy = malloc(sizeof(ulist));
	assert(copy != NULL);

	copy->last = original->last;

	if (original->array != NULL) {
		copy->array = malloc(sizeof(unsigned long long)*(copy->last + 1));
		assert(copy->array != NULL);

		unsigned long long i;
		for (i=0; i<=copy->last; i++) {
			copy->array[i] = original->array[i];
		}
	} else
		copy->array = NULL;

	return (copy);
}

int ulist_remove(ulist* ulist_ptr, unsigned long long element)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
	assert(j <= ulist_ptr->last);
#endif

	if (ulist_ptr->array != NULL) {
		if (ulist_ptr->last == 0) {
			free(ulist_ptr->array);
			ulist_ptr->array = NULL;
		} else {
			unsigned long long i;
			for (i = element; i < ulist_ptr->last; i++) {
				ulist_ptr->array[i] = ulist_ptr->array[i+1];
			}
			ulist_ptr->last = ulist_ptr->last - 1;
		}
	}
	return(0);
}

int ulist_pop(ulist* ulist_ptr, unsigned long long number)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
#endif

	if (ulist_ptr->array != NULL) {
		unsigned long long i;
		for (i = 0; i <= ulist_ptr->last; i++) {
			if (ulist_ptr->array[i] == number) {
				if (ulist_ptr->last == 0) {
					free(ulist_ptr->array);
					ulist_ptr->array = NULL;
				}
				else{
					for (; i < ulist_ptr->last; i++) {
						ulist_ptr->array[i] = ulist_ptr->array[i+1];
					}
					ulist_ptr->last = ulist_ptr->last - 1;
				}
				return (1);
			}
		}
	}
	return(0);
}

unsigned long long ulist_combine_digits(ulist *digits)
{
#ifdef __SANITY_CHECK__
	assert(digits != NULL);
#endif

	unsigned long long number = 0;
	if (digits->array != NULL) {
		unsigned long long i;
		for (i = 0; i <= digits->last; i++) {
			number = (number * 10)+ digits->array[i];
		}
	}
	return (number);
}

unsigned long long ulist_islastzero(ulist *ulist_ptr)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
#endif

	if (ulist_ptr->array != NULL) {
		return(ulist_ptr->array[ulist_ptr->last] == 0);
	}
	return (0);
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
	llist *temp = NULL;
	while(llist_ptr != NULL) {
		temp = llist_ptr;
		llist_ptr = llist_ptr->next;
		free(temp);
	}
	return (0);
}

llist *get_fangs(unsigned long long dividend)
{
	llist *divisors = NULL;
	llist *current = NULL;
	unsigned long long dividend_length = u_length(dividend);
	unsigned long long divisor = ten_pow((dividend_length / 2) - 1);
	unsigned long long divisor_length;
	unsigned long long quotient;
	unsigned long long quotient_length;

	for (; divisor * divisor <= dividend; divisor++) {
		if (dividend % divisor == 0) {
			quotient = dividend/divisor;
			divisor_length = u_length(divisor);
			quotient_length = u_length(quotient);
			if ((dividend_length == divisor_length + quotient_length) && (divisor_length == quotient_length)) {
				if (current == NULL) {
					divisors = llist_init(divisor, NULL);
					current = divisors;
				}
				else{
					current->next = llist_init(divisor, current->next);
					current = current->next;
				}
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
		llist *i;
		for (i = original->next; i != NULL; i = i->next) {
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
	return (0);
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
	llist *i;
	for (i = llhead_ptr->first; i != NULL; i = i->next) {
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
			return (1);
		}
		else
			prev = i;
	}
	return (0);
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
	llhead *result;
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
	new->result = llhead_init(NULL, NULL);
	return new;
}

int vargs_free(vargs *vargs_ptr)
{
	llhead_free(vargs_ptr->result);
	free (vargs_ptr);
	return (0);
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

	return(current);
}

/*----------------------------------------------------------------------------*/

void *vampire(void *void_args)
{

#ifdef __SPEED_TEST___
	struct timespec start, finish;
	double elapsed;
	clock_gettime(__CLOCK_MODE__, &start);
#endif

	vargs *args = (vargs *)void_args;
	unsigned long long is_not_vampire;
	unsigned long long is_vampire;

	/* iterate all numbers */
	unsigned long long number;
	for (number = args->min; number <= args->max; number += args->step) {
		if (u_length_isodd(number)) {
			continue;
		}
		is_vampire = 0;
#ifdef __SPEED_TEST___
	struct timespec al_start, al_finish;
	double al_elapsed;
	clock_gettime(__CLOCK_MODE__, &al_start);
#endif
		llist *divisors = get_fangs(number);
#ifdef __SPEED_TEST___
	clock_gettime(__CLOCK_MODE__, &al_finish);

	al_elapsed = (al_finish.tv_sec - al_start.tv_sec);
	al_elapsed += (al_finish.tv_nsec - al_start.tv_nsec) / 1000000000.0;
	args->algorithm_runtime += al_elapsed;

#endif
		ulist *digits = ulist_init(number);

		llist *i;
		for (i = divisors; i != NULL && !is_vampire; i = i->next) { /* iterate all divisors */

			ulist *digit = ulist_copy(digits);
			ulist* divisor = ulist_init(i->number);
			ulist* quotient = ulist_init(number / i->number);

			is_not_vampire = 0;
			/* iterate all divisor digits */
			unsigned long long j;
			for (j = 0; !is_not_vampire && j <= divisor->last && !is_not_vampire && digit->array != NULL; j++) {
				if (!ulist_pop(digit,divisor->array[j]))
					is_not_vampire = 1;
			}

			for (j = 0; !is_not_vampire && j <= quotient->last && !is_not_vampire && digit->array != NULL; j++) {
				if (!ulist_pop(digit,quotient->array[j]))
					is_not_vampire = 1;
			}
			if (!is_not_vampire && digit->array == NULL  && (!ulist_islastzero(divisor) || !ulist_islastzero(quotient))) {
#if defined(__OEIS_OUTPUT__)
				if (args->result->first == NULL) {
					args->result->first = llist_init(number, NULL);
					args->result->last = args->result->first;
				} else {
					args->result->last->next = llist_init(number, NULL);
					args->result->last = args->result->last->next;
				}
#endif
				args->count ++;
				is_vampire = 1;
				/*
				printf("%llu %llu\n", args->count, number);
				printf("%llu / %llu = %llu\n", number, i->number, number / i->number);
				*/
			}

			ulist_free(digit);
			ulist_free(divisor);
			ulist_free(quotient);
		}
		llist_free(divisors);
		ulist_free(digits);
	}

#ifdef __SPEED_TEST___
	clock_gettime(__CLOCK_MODE__, &finish);
#endif

#ifdef __SPEED_TEST___
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	args->runtime += elapsed;
	/*
	printf("%llu \t%llu \t%llu\tin %lf\n", args->min, args->max, counter, elapsed);
	*/
#endif

	return(0);
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("Usage: vampire [min] [max]\n");
		return (0);
	}
	unsigned long long i;
	unsigned long long min = atoull(argv[1]);
	unsigned long long max = atoull(argv[2]);

	unsigned long long new_min = 1;
	unsigned long long min_length = u_length(min);
	for (i = 0; i < min_length; i++) {
		new_min = (new_min * 10);
	}
	if (u_length_isodd(min)) {
		if (new_min <= max) {
			min = new_min;
		}
	}
	unsigned long long lmax = max;
	if (u_length_isodd(max)) {
		unsigned long long new_max = 0;
		unsigned long long max_length = u_length(max);
	
		for (i = 0; i < max_length - 1; i++) {
			new_max = (new_max * 10) + 9;
		}
		if (min <= new_max) {
			max = new_max;
		}
	}
	unsigned long long lmin = min;
	if (lmax > 10 * new_min -1) {
		lmax = 10 * new_min -1;
	}

	int rc;
	pthread_t threads[NUM_THREADS];
	vargs *input[NUM_THREADS];
	unsigned long long thread;
	for (i = 0; i < NUM_THREADS; i++) {
		input[i] = vargs_init(0, 0, 1);
	}
	llhead *result_list = llhead_init(NULL, NULL);
	unsigned long long iterator = ITERATOR;
	unsigned long long active_threads = NUM_THREADS;

	for (; lmax <= max;) {
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
#if defined(__OEIS_OUTPUT__)
				if (input[thread]->result->first != NULL) {
					if (result_list->first == NULL) {
						result_list->first = input[thread]->result->first;
						result_list->last = input[thread]->result->last;
					} else {
						result_list->last->next = input[thread]->result->first;
						result_list->last = input[thread]->result->last;
					}
				}
				input[thread]->result->first = NULL;
				input[thread]->result->last = NULL;
#endif
			}
		}
		if (lmin * 100 <= max)
			lmin *= 100;

		if (lmax == max)
			break;

		else if (lmax *100 <= max)
			lmax = lmax * 100 + 99;

		else if (lmax != max)
			lmax = max;

		if (lmax-lmin > ITERATOR)
			iterator = ITERATOR;

		else
			iterator = lmin -lmax;

	}
	unsigned long long result = 0;
	double algorithm_time = 0.0;
	for (thread = 0; thread<NUM_THREADS; thread++) {
		result += input[thread]->count;

#ifdef __SPEED_TEST___
		printf("%llu \t%llu \t%llu\tin %lf\n", input[thread]->min, input[thread]->max, input[thread]->count, input[thread]->runtime);
		algorithm_time += input[thread]->algorithm_runtime,

#endif
		vargs_free(input[thread]);
	}
#ifdef __SPEED_TEST___
	printf("algorithm took: %lf\n", algorithm_time);
#endif
#if defined(__OEIS_OUTPUT__)
	unsigned long long k = 1;
	llist *temp;
	for (temp = result_list->first; temp != NULL; temp = temp->next) {
		printf("%llu %llu\n", k, temp->number);
		k++;
	}
#endif

#if defined(__RESULT___)
	printf("found: %llu\n", result);
#endif
	llhead_free(result_list);

	pthread_exit(NULL);

	return (0);
}
