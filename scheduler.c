#define _GNU_SOURCE
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

#undef malloc
#undef free
#undef printf

void * safe_mem(int op, void * arg) {
	static AO_TS_t spinlock = AO_TS_INITIALIZER;
	void * result = 0;

	spinlock_lock(&spinlock);
	if(op == 0) {
	result = malloc((size_t)arg);
	} else {
	free(arg);
	}
	spinlock_unlock(&spinlock);
	return result;
}

int safe_printf(const char* format, ...)
{
	static AO_TS_t spinlock_print = AO_TS_INITIALIZER;
	int result = 0;		
	va_list args;
	va_start (args, format);
	spinlock_lock(&spinlock_print);
	result = vprintf(format,args);
	spinlock_unlock(&spinlock_print);
	va_end(args);
	return result;
}

#define malloc(arg) safe_mem(0, ((void*)(arg)))
#define free(arg) safe_mem(1, arg)
#define printf(format,...) safe_printf(format,__VA_ARGS__)

//struct thread_t * current_thread;//use macro instead
struct queue ready_list;
size_t stack_size = 100000;

AO_TS_t spinlock_readylist = AO_TS_INITIALIZER;//for manipulate ready list

int kernel_thread_begin(void* arg)
{
	//create an empty thread table entry
	struct thread_t* kernel_current_thread = malloc(sizeof(struct thread_t));
	//set its state to RUNNING
	kernel_current_thread->state = RUNNING;
	//set the current thread to that thread table entry
	set_current_thread(kernel_current_thread);
	
	//yield forever whenever the ready list is not empty
	while(1)
	{
		// if (!is_empty(&ready_list))//remove this because checking is embedded in yield()
			yield();
	}
}
	
void scheduler_begin(int N)//N is the number of kernel threads (assume: N>0)
{
	//initialize current thread
	set_current_thread(malloc(sizeof(struct thread_t)));
	current_thread->state = RUNNING;
	
	//initialize ready list
	ready_list.head = ready_list.tail = NULL;
	
	//create kernel threads
	int i = 0;
	for (i=0; i<N-1; i++)
	{
		void* stack = malloc(stack_size);		
		clone(kernel_thread_begin, stack+stack_size-1, CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FILES | CLONE_FS | CLONE_IO, NULL);
	}
}

void scheduler_end()
{
	//placing an infinite loop at the end of our main thread that continues until all other running threads are finished(is_empty queue predicate useful.)
	while (1)
	{
		spinlock_lock(&spinlock_readylist);
		if (!is_empty(&ready_list))
		{
			spinlock_unlock(&spinlock_readylist);
			yield();			
		}
		else
		{
			spinlock_unlock(&spinlock_readylist);
			break;
		}
	}
	// printf("%s\n", "scheduling ended");
}

struct thread_t* thread_fork(void(*target)(void*), void * arg)
{
	//lock ready lock
	spinlock_lock(&spinlock_readylist);
	
	struct thread_t * old_thread;
	struct thread_t * new_thread;
	//Allocate a new thread table entry, and allocate its control stack.
	new_thread = malloc(sizeof(struct thread_t));
	void* stack = malloc(stack_size);
	new_thread->stack_pointer = stack+stack_size-1; 
	
	//Set the new thread's initial argument and initial function.
	new_thread->initial_function = target;
	new_thread->initial_arg =  arg;

	//Set the current thread's state to READY and enqueue it on the ready list.
	current_thread->state = READY;
	thread_enqueue(&ready_list, current_thread);
	
	//Set the new thread's state to RUNNING.
	new_thread->state = RUNNING;
	
	//Save a pointer to the current thread in a temporary variable, then set the current thread to the new thread.
	old_thread = current_thread;
	set_current_thread(new_thread);
	
	//Call thread_start with the old current thread as old and the new current thread as new.
	thread_start(old_thread, current_thread);
	
	//unlock ready lock
	spinlock_unlock(&spinlock_readylist);
	
	return new_thread;//improve: for constructing barrier
}

void thread_free(struct thread_t* t)//improve: free thread's memory
{
	if (t != NULL)
	{
		free((void*)(t->stack_pointer-stack_size+1+128));//+128: to get back to memory address returned by malloc initially		
		free(t);
	}
}


void yield()
{
	//lock ready lock
	spinlock_lock(&spinlock_readylist);
	//dequeue a thread from ready queue and switch (if possible)
	dequeue_and_switch();
}

void block(AO_TS_t * spinlock)
{
	//lock ready lock
	spinlock_lock(&spinlock_readylist);
	//unlock the spinlock 
	spinlock_unlock(spinlock);
	//dequeue a thread from ready queue and switch (if possible)
	dequeue_and_switch();
}

void dequeue_and_switch()//only called by yield and block function
{
	if (!is_empty(&ready_list))//only yield if read_list is not empty
	{
		struct thread_t * next_current_thread;
		struct thread_t * old_current_thread;
		
		//If the current thread is not DONE and not BLOCKED, set its state to READY and enqueue it on the ready list.
		if (current_thread->state != DONE && current_thread->state != BLOCKED)
		{			
			current_thread->state = READY;
			thread_enqueue(&ready_list,current_thread);		
		}
		
		//Dequeue the next thread from the ready list
		next_current_thread = thread_dequeue(&ready_list);
		
		if (next_current_thread)
		{		
			//set its state to RUNNING.
			next_current_thread->state = RUNNING;
			
			//Save a pointer to the current thread in a temporary variable, then set the current thread to the next thread.
			old_current_thread = current_thread;
			set_current_thread(next_current_thread);
			
			//Call thread_switch with the old current thread as old and the new current thread as new
			thread_switch(old_current_thread, current_thread);
			
			//unlock ready lock
			spinlock_unlock(&spinlock_readylist);
		}
		else
		{
			spinlock_unlock(&spinlock_readylist);			
			printf("%s\n", "ERROR in scheduling");			
		}	
	}
	else
		spinlock_unlock(&spinlock_readylist);
}

void thread_wrap(void(*target)(void*), void * arg)
{		
	
	//unlock ready lock	
	spinlock_unlock(&spinlock_readylist);
	
	//call thread initial function
	target(arg);
	// printf("end initial function\n");
	//set current thread's state to DONE
	current_thread->state = DONE;	
	//call yield
	yield();
	
}

/*
void thread_finish()
{
	//printf("%s\n", "finish");
	//set current thread's state to DONE
	current_thread->state = DONE;
	
	//call yield
	yield();
}
*/


void mutex_init(struct mutex * m)
{
	m->holding_thread = NULL;
	m->waiting_threads.head =  m->waiting_threads.tail = NULL;
	m->s = AO_TS_INITIALIZER;
}

void mutex_lock(struct mutex * m)
{
	spinlock_lock(&(m->s));
	if (m->holding_thread == NULL)
	{
		m->holding_thread = current_thread;		
		spinlock_unlock(&(m->s));
	}
	else
	{
		current_thread->state = BLOCKED;
		thread_enqueue(&(m->waiting_threads), current_thread);	
		block(&(m->s));		
	}
}

void mutex_unlock(struct mutex * m)
{
	spinlock_lock(&(m->s));
	//wake up at least a thread waiting for the lock
	struct thread_t * waiting_thread = NULL;
	waiting_thread = thread_dequeue(&(m->waiting_threads));
	//check if there is any waiting thread
	if (waiting_thread)
	{
		spinlock_lock(&spinlock_readylist);
		
		m->holding_thread = waiting_thread;
		waiting_thread->state = READY;
		thread_enqueue(&ready_list,waiting_thread);
		
		spinlock_unlock(&spinlock_readylist);
		spinlock_unlock(&(m->s));
	}
	else
	{
		m->holding_thread = NULL;
		spinlock_unlock(&(m->s));
	}	
}

void condition_init(struct condition * c)
{
	c->waiting_threads.head =  c->waiting_threads.tail = NULL;
	c->s = AO_TS_INITIALIZER;
}

void condition_wait(struct condition * c)
{
	spinlock_lock(&(c->s));
	
	current_thread->state = BLOCKED;
	thread_enqueue(&(c->waiting_threads), current_thread);	
	
	block(&(c->s));
}

void condition_signal(struct condition * c)
{
	spinlock_lock(&(c->s));
	//wake up a thread waiting for the condition
	struct thread_t * waiting_thread = NULL;
	waiting_thread = thread_dequeue(&(c->waiting_threads));
	//check if there is any waiting thread
	if (waiting_thread)
	{
		spinlock_lock(&spinlock_readylist);
		
		waiting_thread->state = READY;
		thread_enqueue(&ready_list,waiting_thread);
		
		spinlock_unlock(&spinlock_readylist);
		spinlock_unlock(&(c->s));
	}
	else
		spinlock_unlock(&(c->s));
}

void condition_broadcast(struct condition * c)
{
	//wake up all threads waiting for the condition	
	while (1)
	{
		spinlock_lock(&(c->s));
		if (!is_empty(&(c->waiting_threads)))
		{
			spinlock_unlock(&(c->s));
			condition_signal(c);
		}
		else
		{
			spinlock_unlock(&(c->s));			
			break;
		}
	}
}

void spinlock_lock(AO_TS_t * lock) {
	while (AO_test_and_set_acquire(lock) == AO_TS_SET)
	{
	}	
}

void spinlock_unlock(AO_TS_t * lock) {
	AO_CLEAR(lock);  
}
