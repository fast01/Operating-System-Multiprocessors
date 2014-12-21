/*
Joshua Stough
Washington and Lee University
Mergesort a random list of size given by the argument (default 1M)
Time both sequential mergesort and parallel (using Pthreads).
See https://computing.llnl.gov/tutorials/pthreads/#Designing

$ gcc -pthread -std=c99 -o mergesort parallelMergesort.c
$ ./mergesort [1000000]
*/

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "scheduler.h"

#define DNUM 1000000
#define THREAD_LEVEL 3

//for sequential and parallel implementation
void merge(double lyst[], double back[], int low, int mid, int high);
void mergesortHelper(double lyst[], double back[], int low, int high);
int isSorted(double lyst[], int size);

//for parallel implementation
void parallelMergesort(double lyst[], int size, int tlevel);
void parallelMergesortHelper(void *threadarg);
struct thread_data{
	double *lyst;
	double *back;
	int low;
	int high;
	int level;
};
//thread_data should be thread-safe, since while lyst and 
//back are shared, [low, high] will not overlap among threads.

int NUM_KERNEL_THREAD;

//for the builtin qsort, for fun:
int compare_doubles (const void *a, const void *b);

/*
Main method:
-generate random list
-time sequential mergesort
-time parallel mergesort
*/
int main (int argc, char *argv[])
{
	struct timeval start, end;
	double diff;

	srand(time(NULL)); //seed random
	
	int NUM = DNUM;
	NUM_KERNEL_THREAD = atoi(argv[1]);
	if (argc == 3) //user specified list size.
	{
		NUM = atoi(argv[2]);
	}

	//Want to compare sorting on the same list,
	//so backup.
	double *lystbck = (double *) malloc(NUM*sizeof(double));
	double *lyst = (double *) malloc(NUM*sizeof(double));
	
	//Populate random original/backup list.
	int i;
	for( i = 0; i < NUM; i ++)
	{
		lystbck[i] = 1.0*rand()/RAND_MAX;
	}

	//copy list.
	memcpy(lyst, lystbck, NUM*sizeof(double));

	gettimeofday(&start, NULL);	
	parallelMergesort(lyst, NUM, THREAD_LEVEL);	
	gettimeofday(&end, NULL);

	if (!isSorted(lyst, NUM))
	{
		printf("%s\n","Oops, lyst did not get sorted by parallelMergesort.");
	}
	
	//Compute time difference.
	diff = ((end.tv_sec * 1000000 + end.tv_usec)
		  - (start.tv_sec * 1000000 + start.tv_usec))/1000000.0;
	printf("Parallel mergesort took: %lf sec.\n", diff);
	
	free(lyst);
	free(lystbck);
}

/*
parallel mergesort top level:
instantiate parallelMergesortHelper thread, and that's 
basically it.
*/
void parallelMergesort(double lyst[], int size, int tlevel)
{
	void *status;

	double *back = (double *) malloc(size*sizeof(double));

	//pthread function can take only one argument, so struct.
	struct thread_data td;
	td.lyst = lyst;
	td.back = back;
	td.low = 0;
	td.high = size - 1;
	td.level = tlevel;
	
	//scheduler begins here
	scheduler_begin(NUM_KERNEL_THREAD);
	struct thread_t* thread = thread_fork(parallelMergesortHelper, (void *) &td);	
	scheduler_end();
	
	free(back);
}

/*
parallelMergesortHelper
-if the level is still > 0, then make 
parallelMergesortHelper threads to solve the left and 
right-hand sides, then merge results after joining
 and quit.
*/
void parallelMergesortHelper(void *threadarg)
{
	int mid, t, rc;
	void *status;

	struct thread_data *my_data;
	my_data = (struct thread_data *) threadarg;
	
	//fyi:
	//printf("Thread responsible for [%d, %d], level %d.\n",
	//		my_data->low, my_data->high, my_data->level);
	
	if (my_data->level <= 0 || my_data->low == my_data->high)
	{
		//We have plenty of threads, finish with sequential.
		mergesortHelper(my_data->lyst, my_data->back, 
						my_data->low, my_data->high);
		return;
	}

	//At this point, we will create threads for the 
	//left and right sides.  Must create their data args.
	struct thread_data thread_data_array[2];
	mid = (my_data->low + my_data->high)/2;

	for (t = 0; t < 2; t ++)
	{
		thread_data_array[t].lyst = my_data->lyst;
		thread_data_array[t].back = my_data->back;
		thread_data_array[t].level = my_data->level - 1;
	}
	thread_data_array[0].low = my_data->low;
	thread_data_array[0].high = mid;
	thread_data_array[1].low = mid+1;
	thread_data_array[1].high = my_data->high;

	//Now, instantiate the threads.
	struct thread_t* threads[2];//pthread_t threads[2];
	for (t = 0; t < 2; t ++)
	{		
		threads[t] = thread_fork(parallelMergesortHelper,(void *) &thread_data_array[t]);
	}
	
	//Now, join the left and right threads and merge.
	while (threads[0]->state != DONE || threads[1]->state !=DONE )
	{
		yield();
	}
	//Call the sequential merge now that the left and right
	//sides are sorted.
	merge(my_data->lyst, my_data->back, my_data->low, mid, my_data->high);	
}

//for the built-in qsort comparator
//from http://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html#Comparison-Functions
int compare_doubles (const void *a, const void *b)
{
	const double *da = (const double *) a;
	const double *db = (const double *) b;
     
	return (*da > *db) - (*da < *db);
}


//merge the elements from the sorted sublysts in [low, mid] and (mid, high]
void merge(double lyst[], double back[], int low, int mid, int high)
{
	int ai = low, bi = mid + 1, i = low;
	while (ai <= mid && bi <= high)
	{
		if (lyst[ai] <= lyst[bi])
		{
			back[i] = lyst[ai];
			ai ++;
		}else
		{
			back[i] = lyst[bi];
			bi ++;
		}
		i ++;
	}
	if (ai > mid)
	{
		memcpy(&back[i], &lyst[bi], (high-bi+1)*sizeof(double));
	}else
	{
		memcpy(&back[i], &lyst[ai], (mid-ai+1)*sizeof(double));
	}
	memcpy(&lyst[low], &back[low], (high-low+1)*sizeof(double));
}

//the actual C mergesort method, with back list to avoid 
//NlogN memory (2N instead).
void mergesortHelper(double lyst[], double back[], int low, int high)
{
	if (low == high) return;
	int mid = (low + high)/2;
	mergesortHelper(lyst, back, low, mid);
	mergesortHelper(lyst, back, mid+1, high);
	merge(lyst, back, low, mid, high);
}


//check if the elements of lyst are in non-decreasing order.
//one is success.
int isSorted(double lyst[], int size)
{
	int i;
	for (i = 1; i < size; i ++)
	{
		if (lyst[i] < lyst[i-1]) 
		{
			printf("%e < %e\n", lyst[i], lyst[i-1]);
			return 0;
		}
	}
	return 1;
}

