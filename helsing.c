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

/*
Compile with:
gcc -O3 helsing.c -o helsing
*/

unsigned int uint_length (unsigned int number){
	int i = 1;
	for(; number > 9; i++){
		number = number / 10;	//Using division to avoid overflow for numbers around 2^32 -1
	}
	return (i);
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

llist *llist_copy(llist *old)
{
	llist *new = NULL;
	if(old != NULL){
		new = llist_init(old->number, NULL);
		llist *current = new;
		for(llist *i = old->next; i != NULL; i = i->next){
			llist *llist_ptr = llist_init(i->number, NULL);
			current->next = llist_ptr;
			current = current->next;
		}
	}
	return (new);
}

//---------------------------------- LLHEAD ----------------------------------//

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
	if(last != NULL)
		new->last = last;
	else
		new->last = new->first;
	return (new);
}

int llhead_free(llhead *llhead_ptr)
{
	if(llhead_ptr != NULL){
		llist_free(llhead_ptr->first);
	}
	free(llhead_ptr);
	return (0);
}

llhead *llhead_copy(llhead *old){
	llist *first = NULL;
	first = llist_copy(old->first);
	llist *last = NULL;	//this works but technically it shouldn't be just NULL.
	llhead *new = llhead_init(first,last);
	return (new);
}

int llhead_pop(llhead *llhead_ptr, unsigned int number)
{
	llist *prev = NULL;
	for(llist *i = llhead_ptr->first; i != NULL; i = i->next){
		if(i->number == number){
			if(i == llhead_ptr->first){
				llhead_ptr->first = i->next;
			}
			if(i == llhead_ptr->last){
				llhead_ptr->last = prev;
			}
			if(prev != NULL){
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

llhead *separate_digits(unsigned int number)
{
	unsigned int digit = number % 10;
	llist *llist_ptr = llist_init(digit, NULL);
	llhead *digits = NULL;
	if(number > 9){
		digits = separate_digits(number / 10);
		digits->last->next = llist_ptr;
		digits->last = llist_ptr;
	}
	else{
		digits = llhead_init(llist_ptr, NULL);
	}
	return digits;
}

unsigned int combine_digits(llist *digits)
{
	unsigned int result = 0;
	for(;digits != NULL; digits = digits->next){
		result = (result * 10)+ digits->number;
	}
	return (result);
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

		llist *divisor_llist = get_divisors(number);
		llhead *digits = separate_digits(number);

		//iterate all divisors
		for(llist *i=divisor_llist; i!=NULL; i=i->next){
			llhead *temp = llhead_copy(digits);
			llhead *divisor_digits = separate_digits(i->number);

			llist *j=divisor_digits->first;


			exit = 0;
			//iterate all divisor digits
			for(; j != NULL && !exit; j = j->next){
				if(!llhead_pop(temp,j->number))
					exit = 1;
			}

			//check result
			unsigned int result = combine_digits(temp->first);

			if(!exit && number / i->number == result ){
				counter ++;
				assert((uint_length(number) > 2) && (uint_length(number) >= uint_length(result) + 2));
			}			

			llhead_free(temp);
			llhead_free(divisor_digits);
		}
		llist_free(divisor_llist);
		llhead_free(digits);
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
