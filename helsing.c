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

#define NUM_THREADS 1

//#define __SANITY_CHECK__

/*
Compile with:
gcc helsing.c -o helsing -O3
*/

unsigned int uint_length (unsigned int number){
	int i = 1;
	for(; number > 9; i++){
		number = number / 10;	//Using division to avoid overflow for numbers around 2^32 -1
	}
	return (i);
}

//---------------------------------- ULIST  ----------------------------------//

typedef struct ulist
{
	unsigned int length;
	unsigned int *array;
} ulist;

ulist *ulist_init(unsigned int number)
{
	ulist *new = malloc(sizeof(ulist));
	assert(new != NULL);

	new->length = uint_length(number) - 1;

	new->array = malloc(sizeof(unsigned int)*(new->length + 1));
	assert(new->array != NULL);

	for(unsigned int i=new->length; i>0; i--){
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

	new->length = old->length;

	new->array = malloc(sizeof(unsigned int)*(new->length + 1));
	assert(new->array != NULL);

	for(unsigned int i=0; i<=new->length; i++){
		new->array[i] = old->array[i];
	}
	return (new);
}

int ulist_remove(ulist* ulist_ptr, unsigned int j)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
	assert(j <= ulist_ptr->length);
#endif

	if(ulist_ptr->length == 0){
		free(ulist_ptr->array);
		ulist_ptr->array == NULL;
	}
	else{
		for(unsigned int i=j; i<ulist_ptr->length; i++){
			ulist_ptr->array[i] = ulist_ptr->array[i+1];
		}
		ulist_ptr->length = ulist_ptr->length - 1;
	}

	return(0);
}

int ulist_pop(ulist* ulist_ptr, unsigned int number)
{
#ifdef __SANITY_CHECK__
	assert(ulist_ptr != NULL);
#endif

	for(unsigned int i=0; i<=ulist_ptr->length; i++){
		if(ulist_ptr->array[i] == number){
			if(ulist_ptr->length == 0){
				free(ulist_ptr->array);
				ulist_ptr->array == NULL;
			}
			else{
				for(; i < ulist_ptr->length; i++){
					ulist_ptr->array[i] = ulist_ptr->array[i+1];
				}
				ulist_ptr->length = ulist_ptr->length - 1;
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
	for(unsigned int i=0; i<=digits->length; i++){
		result = (result * 10)+ digits->array[i];
	}
	return (result);
}

//---------------------------------- LLIST  ----------------------------------//

typedef struct llist
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

//----------------------------------------------------------------------------//

llist *get_divisors(unsigned int dividend)
{
	llist *divisors = NULL;
	llist *current = NULL;
	llist *llist_ptr, *llist_ptr2, *llist_temp;
	unsigned int quotient;
	unsigned int number_length = uint_length(dividend);

	for(unsigned int i = 2; i * i <= dividend; i++){
		if(dividend % i == 0){
			quotient = dividend/i;
			if(number_length == uint_length(i) + uint_length(quotient)){
				llist_temp = NULL;
				if(current != NULL)
					llist_temp = current->next;

				//if((number_length > 2) && (number_length >= uint_length(quotient) + 2)){
				//	printf("%d/%d=%d\n", dividend, i, quotient);
				//	llist_ptr2 = llist_temp;
				//}
				//else{
					llist_ptr2 = llist_init(quotient , llist_temp);
				//	printf(" %d/%d=%d\n", dividend, i, quotient);
				//}
				llist_ptr = llist_init(i, llist_ptr2);

				if(divisors == NULL){
					divisors = llist_ptr;
				}
				else{
					current->next = llist_ptr;
				}
				current = llist_ptr;
			}
		}
	}
	return (divisors);
}

//----------------------------------------------------------------------------//

typedef struct voperators
{
	unsigned int min;
	unsigned int max;
	unsigned int step;
} voperators;

voperators *voperators_init(unsigned int min, unsigned int max, unsigned int step)
{
	voperators *new = malloc(sizeof(voperators));
	assert(new != NULL);
	new->min = min;
	new->max = max;
	new->step = step;
	return new;
}

unsigned int vamparker(voperators *input){
	unsigned int counter= 0;
	unsigned int exit;

	//iterate all numbers
	for(unsigned int number = input->min ;number <= input->max; number += input->step){

		llist *divisors = get_divisors(number);
		ulist *digits = ulist_init(number);

		//iterate all divisors
		for(llist *i=divisors; i!=NULL; i=i->next){
			ulist *digit = ulist_copy(digits);
			ulist* divisor = ulist_init(i->number);

			exit = 0;
			//iterate all divisor digits
			for(int j = 0; j<=divisor->length && !exit ;j++){
				if(!ulist_pop(digit,divisor->array[j]))
					exit = 1;
			}

			//check result
			unsigned int result = ulist_combine_digits(digit);

			if(!exit && number / i->number == result ){
				counter ++;
				//assert((uint_length(number) > 2) && (uint_length(number) >= uint_length(result) + 2));
			}			

			ulist_free(digit);
			ulist_free(divisor);
		}
		llist_free(divisors);
		ulist_free(digits);
	}
	return (counter);
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

	clock_t time = clock();

	voperators input;
	input.min = min;
	input.max = max;
	input.step = 1;

	unsigned int counter = vamparker(&input);

	time = clock() - time;
	double runtime = (double)(time) / CLOCKS_PER_SEC;

	printf("%u \t%u \t%u\tin %lf\n", min, max, counter, runtime);
/*
	pthread_t threads[NUM_THREADS];
	int rc;
	int i;
	for( i = 0; i < NUM_THREADS; i++ ) {
		rc = pthread_create(&threads[i], NULL, therad_test, (void *)&i);
		if (rc) {
			printf("Error:unable to create thread, %d\n", rc);
			exit(-1);
		}
	}
   pthread_exit(NULL);
*/
	return (0);
}


//----------------------------------------------------------------
//		clock_t begin, end;
//		double runtime = 0.0;
//		begin = clock();

//		end = clock();
//		runtime += (double)(end-begin) / CLOCKS_PER_SEC;
//----------------------------------------------------------------
