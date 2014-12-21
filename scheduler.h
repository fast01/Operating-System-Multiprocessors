#include <atomic_ops.h>
#include "queue.h"

extern struct thread_t * get_current_thread();
extern void set_current_thread(struct thread_t*);
extern void* safe_mem(int, void*);

//macro
#define current_thread (get_current_thread())
#define malloc(arg) safe_mem(0, ((void*)(arg)))
#define free(arg) safe_mem(1, arg)
#define printf(format, ...) safe_printf(format, __VA_ARGS__)

//define thread state
typedef enum {
	RUNNING,
	READY,
	BLOCKED,
	DONE
} state_t;

//define thread table entry struct
struct thread_t
{
	void* stack_pointer;	
	void (*initial_function) (void*);
	void* initial_arg;
	state_t state;
	
};

//define mutex struct
struct mutex
{
	struct thread_t * holding_thread;
	struct queue waiting_threads;
	AO_TS_t s;
};

//define condition variable struct
struct condition
{
	struct queue waiting_threads;
	AO_TS_t s;
};


void scheduler_begin(int);
void scheduler_end();
struct thread_t* thread_fork(void(*target)(void*), void * arg);
void yield();
void thread_wrap(void(*target)(void*), void * arg);
void block(AO_TS_t * spinlock);
void dequeue_and_switch();//only called by yield and block function
// void thread_finish();

//improve:
void thread_free(struct thread_t*);


void mutex_init(struct mutex *);
void mutex_lock(struct mutex *);
void mutex_unlock(struct mutex *);

void condition_init(struct condition *);
void condition_wait(struct condition *);
void condition_signal(struct condition *);
void condition_broadcast(struct condition *);

void spinlock_lock(AO_TS_t *);
void spinlock_unlock(AO_TS_t *);
