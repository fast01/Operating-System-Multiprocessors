/*
 * CS533 Course Project
 * Thread Queue ADT
 * queue.h
 *
 * Feel free to modify this file. Please thoroughly comment on
 * any changes you make.
 */

struct queue_node {
  struct thread_t * t;
  struct queue_node * next;  
};

struct queue {
  struct queue_node * head;
  struct queue_node * tail;
};

void thread_enqueue(struct queue * q, struct thread_t * t);
struct thread_t * thread_dequeue(struct queue * q);
int is_empty(struct queue * q); //check if queue is empty (return 1) or not (return 0)
