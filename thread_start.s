#void thread_start(thread_t * old, thread_t * new);

.globl thread_start

thread_start:
	#push all callee-save registers onto the current stack
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	
	#save the current stack pointer (%rsp) in old's thread table entry 
	movq %rsp, (%rdi)
	 
	#load the stack pointer from new's thread table entry into %rsp
	movq (%rsi), %rsp
	
	#pass our initial function to the target function of thread_wrap
	movq 8(%rsi), %rdi
	
	#pass our initial argument to argument of thread_wrap
	movq 16(%rsi), %rsi
			
	#jump to thread_wrap	
	jmp thread_wrap
