//source of producer/consumer:
//http://web.stanford.edu/class/cs140/cgi-bin/lecture.php?topic=locks
#define _GNU_SOURCE
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "scheduler.h"

#define SIZE 3
char buffer [SIZE];
int count = 0, head = 0, tail = 0;
struct mutex m;
struct condition notEmpty;
struct condition notFull;

void put(char c) {
    mutex_lock(&m);
    while (count == SIZE) {
		mutex_unlock(&m);
        condition_wait(&notFull);
		mutex_lock(&m);
    }
    count++;
    buffer[head] = c;
    head++;
    if (head == SIZE) {
        head = 0;
    }
    condition_signal(&notEmpty);
    mutex_unlock(&m);
}

char get() {
    char c;
    mutex_lock(&m);
    while (count == 0) {
		mutex_unlock(&m);
        condition_wait(&notEmpty);
		mutex_lock(&m);
    }
    count--;
    c = buffer[tail];
	buffer[tail] = 'X';//set this slot back to empty (for testing easily) 
    tail++;
    if (tail == SIZE) {
        tail = 0;
    }
    condition_signal(&notFull);
    mutex_unlock(&m);
    return c;
}

void producer(void* ID)
{
	int i;
	for (i = 0; i< SIZE; i++)
	{
		put((char)((int)'0')+i);
		printf("kernel_thread: %ld, Producer %s puts: %d, buffer: %s\n", syscall(SYS_gettid), (char*)ID, i, buffer);		
		yield();
	}
}

void consumer(void* ID)
{
	int i;
	char c;
	for (i = 0; i< SIZE*2; i++)
	{
		c = get();		
		printf("kernel_thread: %ld, Consumer %s gets: %c, buffer: %s\n", syscall(SYS_gettid), (char*)ID, c, buffer);	
		yield();
	}
}

int main(void)
{	
	mutex_init(&m);
	condition_init(&notEmpty);
	condition_init(&notFull);
	memset(buffer, (int)'X', SIZE);//init buffer: X means empty slot (for testing easily)	
	scheduler_begin(5);
    struct thread_t* thread0 = thread_fork(producer, (void*)"1");
	struct thread_t* thread1 = thread_fork(producer, (void*)"2");
	struct thread_t* thread2 = thread_fork(consumer, (void*)"1");	
    // thread_fork(consumer, (void*)"2");
	scheduler_end(); 
	thread_free(thread0);
	thread_free(thread1);
	thread_free(thread2);
	// printf("%s\n", "main_thread: return");
}
