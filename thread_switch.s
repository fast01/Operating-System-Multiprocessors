#void thread_switch(thread_t * old, thread_t * new);

.globl thread_switch

thread_switch:
	#push all callee-save registers on the current stack
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	
	#save the current stack pointer in old's thread table entry
	movq %rsp, (%rdi)
	
	#load the stack pointer from new's thread table entry
	movq (%rsi), %rsp
	
	#pop all callee-save registers from the new stack
	popq %r15 
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx
		
	#return
	ret
