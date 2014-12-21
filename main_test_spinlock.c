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
extern AO_TS_t spinlock_print;
AO_TS_t spinlock_m = AO_TS_INITIALIZER;

//increment a shared counter (n: thread ID)
void increment(void * n) 
{	
	int i;
	for (i =0;i<3;i++)
	{		
		spinlock_lock(&spinlock_m);		
		int temp = shared_counter;
		sleep(1);//for testing: making data race happen easier without acquiring spinlock		
		shared_counter = temp + 1;							
		printf("kernel_thread %ld: user_thread %s: shared_counter = %d\n",syscall(SYS_gettid),(char*)n, shared_counter);	
		spinlock_unlock(&spinlock_m);
		yield();//should not use inside spinlock_lock and spinlock_unlock of a lock  to prevent possible deadlock
	}
}

int main(void)
{		
	scheduler_begin(2);
    	thread_fork(increment, (void*)"1");
	thread_fork(increment, (void*)"2");	
	thread_fork(increment, (void*)"3");
	thread_fork(increment, (void*)"4");
	thread_fork(increment, (void*)"5");
	scheduler_end(); 
	// printf("%s\n", "main_thread: return");
}
