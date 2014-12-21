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
	for (i =0;i<300;i++)
	{
		mutex_lock(&m);
		int temp = shared_counter;
		//sleep(1);
		yield();
		shared_counter = temp + 1;		
		//printf("kernel_thread %ld: user_thread %s: shared_counter = %d\n",syscall(SYS_gettid),(char*)n, shared_counter);		
		mutex_unlock(&m);
	}
}

int main(void)
{	
	mutex_init(&m);
	
	scheduler_begin(5);
    thread_fork(increment, (void*)"1");
	thread_fork(increment, (void*)"2");	
	thread_fork(increment, (void*)"3");
	thread_fork(increment, (void*)"4");
	thread_fork(increment, (void*)"5");
    scheduler_end(); 
	printf("shared_counter = %d\n", shared_counter);		
	// printf("%s\n", "main_thread: return");
}
