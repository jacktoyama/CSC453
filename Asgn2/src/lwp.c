#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>

#include "lwp.h"

/* LWP Library */

// typedef int (*lwpfun)(void *); reads as lwpfun is a type for a ptr to a function that takes a void * arg and returns int

tid_t lwp_create(lwpfun fun, void *args) {
	struct rlimit lim;

	// Find stack size	
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size == -1) {
		fprintf(stderr, "Error: sysconf failed!\n");
	} else {
		printf("System page size: %ld bytes\n", page_size);
	}

	// Grab stack size resource limit
	long soft_limit;
	if (getrlimit(RLIMIT_STACK, &lim) == 0) {
		soft_limit = (long)lim.rlim_cur; // Get soft litmit of stack		
		if (soft_limit == RLIM_INFINITY) {
			soft_limit = 8388608; // 8MB bytes	
		}
		printf("Soft limit: %ld\n", soft_limit);
	} else {
		fprintf(stderr, "Error: getrlimit failed!\n");
	}

	unsigned long t_num = 1;

	// NOTE: mmap returns the start of the mapped region; aka lowest addy/base/bottom of stack).
	// Stack grows downward (high address to low address)
	
	// Create a thread object
	thread t = malloc(sizeof(context)); // gets access to context struct
	t->tid = t_num;
	t->stack = mmap(NULL, soft_limit, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);	
	t->stacksize = soft_limit; // 8388608 bytes or 8MB
	t_num++; // Increment the next thread number
	
	// Ex:  t->stack = 0x10000000
	// 	t->stacksize = 0x008388608
	// 	sp_to_tos = 0x108388608
	// 	 	    -----------
	//                  -----------
	// 	bp_to_stack 0x100000000
	sp_to_tos = t->stack + t->stacksize; // top of stack (end of the allocated region)
	
	t->state.fxsave = FPU_INIT; // Initialize predefined FPU_INIT value
	// mmapp stack for threads
}

int main(void) {
	struct rlimit lim;

	// Find stack size	
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size == -1) {
		fprintf(stderr, "Error: sysconf failed!\n");
	} else {
		printf("System page size: %ld bytes\n", page_size);
	}

	// Grab stack size resource limit
	long soft_stack_limit;
	if (getrlimit(RLIMIT_STACK, &lim) == 0) {
		soft_stack_limit = (long)lim.rlim_cur; // Get soft litmit of stack		
		printf("Soft limit: %ld\n", soft_stack_limit);
	} else {
		fprintf(stderr, "Error: getrlimit failed!\n");
	}


	return 0;
}
