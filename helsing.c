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

#define NUM_THREADS 8
//#define __SANITY_CHECK__
//#define __OEIS_OUTPUT__
#define __SPEED_TEST__

/*
Compile with:
gcc -O3 helsing.c -o helsing
*/

unsigned int uint_length (unsigned int number){
	int i = 1;
	for(; number > 9; i++){
		number = number / 10;	// Using division to avoid overflow for numbers around 2^32 -1
	}
	return (i);
}

//---------------------------------- ULIST  ----------------------------------//


typedef struct ulist	// List of unsigned integes
{
	unsigned int *array;	// Set to NULL when the array is empty. Check if NULL before traversing the array!
	unsigned int last_element;	// Code should traverse the array up to [last_element]. May be smaller or equal to the actual size of the array.
} ulist;

ulist *ulist_init(unsigned int number)
{
	ulist *new = malloc(sizeof(ulist));
	assert(new != NULL);

	new->last_element = uint_length(number) - 1;

	new->array = malloc(sizeof(unsigned int)*(new->last_element + 1));
	assert(new->array != NULL);

	for(unsigned int i=new->last_element; i>0; i--){
		new->array[i] = number % 10;
		number = number/10;
	}
	new->array[0] = number % 10;
	
	return (new);
}

int ulist_free(ulist* ulist_ptr)
{
	free(ulist_ptr->array);
	free(ulist_ptr);
	return (0);
}

ulist *ulist_copy(ulist *old)
{
#ifdef __SANITY_CHECK__
	assert(old != NULL);
#endif

	ulist *new = malloc(sizeof(ulist));
	assert(new != NULL);

	new->last_element = old->last_element;

	new->array = malloc(sizeof(unsigned int)*(new->last_element + 1));
	assert(new->array != NULL);

	for(unsigned int i=0; i<=new->last_element; i++){
		new->array[i] = old->array[i];
	}
	return (new);
}

int ulist_remove(ulist* ulist_ptr, unsigned int j)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
	assert(j <= ulist_ptr->last_element);
#endif

	if(ulist_ptr->last_element == 0){
		free(ulist_ptr->array);
		ulist_ptr->array == NULL;
	}
	else{
		for(unsigned int i=j; i<ulist_ptr->last_element; i++){
			ulist_ptr->array[i] = ulist_ptr->array[i+1];
		}
		ulist_ptr->last_element = ulist_ptr->last_element - 1;
	}

	return(0);
}

int ulist_pop(ulist* ulist_ptr, unsigned int number)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
#endif

	for(unsigned int i=0; i<=ulist_ptr->last_element; i++){
		if(ulist_ptr->array[i] == number){
			if(ulist_ptr->last_element == 0){
				free(ulist_ptr->array);
				ulist_ptr->array == NULL;
			}
			else{
				for(; i < ulist_ptr->last_element; i++){
					ulist_ptr->array[i] = ulist_ptr->array[i+1];
				}
				ulist_ptr->last_element = ulist_ptr->last_element - 1;
			}
			return (1);
		}
	}
	return(0);
}

unsigned int ulist_combine_digits(ulist *digits)
{
#ifdef __SANITY_CHECK__
	assert(digits != NULL);
#endif

	unsigned int result = 0;
	for(unsigned int i=0; i<=digits->last_element; i++){
		result = (result * 10)+ digits->array[i];
	}
	return (result);
}

//---------------------------------- LLIST  ----------------------------------//

typedef struct llist	// Linked list of unsigned integers
{
	unsigned int number;
	struct llist *next;
} llist;

llist *llist_init(unsigned int number, llist* next)
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
	while(llist_ptr != NULL){
		temp = llist_ptr;
		llist_ptr = llist_ptr->next;
		free(temp);
	}
	return (0);
}

llist *get_divisors(unsigned int dividend)
{
	llist *divisors = NULL;
	llist *current = NULL;
	llist *llist_temp;
	unsigned int quotient;
	unsigned int number_length = uint_length(dividend);

	unsigned int divisor = 2;
	for(; divisor * divisor <= dividend && current == NULL; divisor++){
		if(dividend % divisor == 0){
			quotient = dividend/divisor;
			if(number_length == uint_length(divisor) + uint_length(quotient)){
				llist_temp = llist_init(quotient , NULL);
				divisors = llist_init(divisor, llist_temp);
				current = divisors;
			}
		}
	}
	for(; divisor * divisor <= dividend; divisor++){
		if(dividend % divisor == 0){
			quotient = dividend/divisor;
			if(number_length == uint_length(divisor) + uint_length(quotient)){
				llist_temp = llist_init(quotient , current->next);
				current->next = llist_init(divisor, llist_temp);
				current = current->next;
			}
		}
	}
	return (divisors);
}

//----------------------------------------------------------------------------//

typedef struct vargs	// Vampire arguments
{
	unsigned int min;
	unsigned int max;
	unsigned int step;
} vargs;

vargs *vargs_init(unsigned int min, unsigned int max, unsigned int step)
{
	vargs *new = malloc(sizeof(vargs));
	assert(new != NULL);
	new->min = min;
	new->max = max;
	new->step = step;
	return new;
}

int vargs_free(vargs * vargs_ptr){
	free (vargs_ptr);
	return (0);
}

//----------------------------------------------------------------------------//

void *vampire(void *args)
{
	struct timespec start, finish;
	double elapsed;

#if defined(__SPEED_TEST__) && defined(CLOCK_MONOTONIC)
	clock_gettime(CLOCK_MONOTONIC, &start);
#elif defined(__SPEED_TEST__) && defined(CLOCK_REALTIME)
	clock_gettime(CLOCK_REALTIME, &start);
#endif

	vargs *input = (vargs *)args;
	unsigned int counter= 0;
	unsigned int exit;
	unsigned int exit_2;

	//iterate all numbers
	for(unsigned int number = input->min ; number <= input->max; number += input->step){

		exit_2 = 0;
		llist *divisors = get_divisors(number);
		ulist *digits = ulist_init(number);

		//iterate all divisors
		for(llist *i=divisors; i!=NULL && !exit_2; i=i->next){
			ulist *digit = ulist_copy(digits);
			ulist* divisor = ulist_init(i->number);

			exit = 0;
			//iterate all divisor digits
			for(int j = 0; j<=divisor->last_element && !exit ;j++){
				if(!ulist_pop(digit,divisor->array[j]))
					exit = 1;
			}

			//check result
			unsigned int result = ulist_combine_digits(digit);

			if(!exit && number / i->number == result ){
				counter ++;
				exit_2 = 1;

#ifdef __OEIS_OUTPUT__
				//printf("%u %u\n", counter, number);
#endif
				//printf("%u / %u = %u\n", number, i->number, result);
				//assert((uint_length(number) > 2) && (uint_length(number) >= uint_length(result) + 2));
			}			

			ulist_free(digit);
			ulist_free(divisor);
		}
		llist_free(divisors);
		ulist_free(digits);
	}


#if defined(__SPEED_TEST__) && defined(CLOCK_MONOTONIC)
	clock_gettime(CLOCK_MONOTONIC, &finish);
#elif defined(__SPEED_TEST__) && defined(CLOCK_REALTIME)
	clock_gettime(CLOCK_REALTIME, &finish);
#endif

#if defined(__SPEED_TEST__) && (defined(CLOCK_MONOTONIC) || defined(CLOCK_REALTIME))
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("%u \t%u \t%u\tin %lf\n", input->min, input->max, counter, elapsed);
#endif	

	//return (counter);
	return(0);
}

void *therad_test(void *threadid){
   long tid;
   tid = (long)threadid;
   printf("Hello World! Thread ID, %ld\n", tid);
   pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	if(argc != 3){
		printf("Usage: vampark [min] [max]\n");
		return (0);
	}

	unsigned int min = atoi(argv[1]);
	unsigned int max = atoi(argv[2]);


//	vargs *input = vargs_init(min, max, NUM_THREADS);

//	vampire(input);


	vargs *input[NUM_THREADS];
	for(int i = 0; i<NUM_THREADS; i++){
		input[i] = vargs_init(i, max, NUM_THREADS);
	}

	int rc;
	pthread_t threads[NUM_THREADS];
	for(int i = 0; i < NUM_THREADS; i++) {
		rc = pthread_create(&threads[i], NULL, vampire, (void *)input[i]);
		if (rc) {
			printf("Error:unable to create thread, %d\n", rc);
			exit(-1);
		}
	}
	pthread_exit(NULL);

	for(int i = 0; i<NUM_THREADS; i++){
		vargs_free(input[i]);
	}

	return (0);
}
