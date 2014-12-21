#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "scheduler.h"


int shared_counter = 0;
struct mutex m;

//increment a shared counter (n: thread ID)
void increment(void * n) 
{	
	int i;
	for (i =0;i<100;i++)
	{
		mutex_lock(&m);
		int temp = shared_counter;
		//sleep(1);
		yield();
		shared_counter = temp + 1;		
		// printf("kernel_thread %ld: user_thread %s: shared_counter = %d\n",syscall(SYS_gettid),(char*)n, shared_counter);				
		mutex_unlock(&m);
	}
}

int test_scheduler()
{
	shared_counter = 0;
	struct thread_t* threads[5];
	scheduler_begin(2);
	threads[0] = thread_fork(increment, (void*)"1");
	threads[1] = thread_fork(increment, (void*)"2");	
	threads[2] = thread_fork(increment, (void*)"3");
	threads[3] = thread_fork(increment, (void*)"4");
	threads[4] = thread_fork(increment, (void*)"5");
	scheduler_end(); 
	//free threads' memories
	int i;
	for (i=0;i<5;i++)
	{
		thread_free(threads[i]);
	}
	//check if the result is correct
	if (shared_counter != 100*5)
	{		
		return -1;
	}			
	return 0;
}
int main(void)
{	
	mutex_init(&m);	
	int i;
	for (i=0;i<100;i++)
	{
		if (test_scheduler()==-1)
		{
			printf("%s\n","Failed");
			return 0;
		}
		printf("Iteration: %d\n",i);
	}
	printf("%s\n","Success!");
}
