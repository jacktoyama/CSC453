#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include <sys/resource.h>
#include "RoundRobin.h"
#include "lwp.h"
#include "DoubleLinkedList.h"

// Global variables
static thread current_thread = NULL;
static scheduler current_scheduler = NULL;
unsigned long t_num = 1;

DoubleLinkedList toTerminate;


/* LWP Library */

// typedef int (*lwpfun)(void *); reads as lwpfun is a type for a ptr to a function that takes a void * arg and returns int
static void lwp_wrap(lwpfun fun, void *arg) {
	/* Call the given lwpfunction with the given argument.
 	 * Call lpw_exit() with its return value
  	 */
	int rval;
	rval = fun(arg);
	lwp_exit(rval);
}

// -----lwp_create()-----
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

	// Round page size in case other machines don't have the exact 8Mb bytes
	long rounded_stacksize = ((soft_limit + page_size -1) / page_size ) * page_size;
		
	// NOTE: mmap returns the start of the mapped region; aka lowest addy/base/bottom of stack).
	// Stack grows downward (high address to low address)
	
	// Create a thread object
	thread t = malloc(sizeof(context)); // gets access to context struct
	if (t == NULL) {
		return NO_THREAD;
	}

	t->tid = t_num++;
	t->stack = mmap(NULL, rounded_stacksize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);	
	if (t->stack == MAP_FAILED) {
		fprintf(stderr, "ERROR: mmap failed!\n");
		free(t);
		return NO_THREAD;
	}
	t->stacksize = rounded_stacksize; // 8388608 bytes or 8MB
	
	// Set stack pointer to the top of the memory chunk
	
	// Stack: High to Low
	// 0x800000 = 1000_0000_0000_0000_0000_0000 = 2^23 = 8388608 (AKA maxHighAddressMemory)
	// ........
	// ........
	// ........
	// 0x000000 = 0000_0000_0000_0000_0000_0000 = 0 (AKA minLowAddressMemory)
	
	/* Increment of 16 bytes...eventually gets to 0x800000
	// 0x001000 = 0000_0000_0001_0000_0000_0000 = 2^12          = 4096
	// 0x001010 = 0000_0000_0001_0000_0001_0000 = 2^12 + 2+4    = 4112
	// 0x001020 = 0000_0000_0001_0000_0010_0000 = 2^12 + 2^5    = 4128
	// ...
	// ...
	// 0x800000
	*/

	// Ex: 0x1003B & ~0xF
	//     0001_0000_0000_0011_1011
	//     &
	//   ~(0000_0000_0000_0000_1111)
	//   = 1111_1111_1111_1111_0000
	//   ---------------------------
	//   = 0001_0000_0000_0011_0000
	//   = 0x10030
	
	// Move by t->stacksize * sizeof(type)
	// So typecast t->stack to type char * so it t->stacksize * sizeof(char) = 8388608 * 1
	// Then re typecase it to unsigned long *
	// 8388608 and 0x800000 are the same value in memory
	unsigned long *s_ptr =  (unsigned long *)((char *)t->stack + t->stacksize); // top of stack (end of the allocated region)
	s_ptr = (unsigned long *)((unsigned long)s_ptr & ~(unsigned long)0xF);

	
	*--s_ptr = 0;
	*--s_ptr = (unsigned long)lwp_wrap;
	*--s_ptr = 0;

	t->state.rsi = (unsigned long)args;
	t->state.rdi = (unsigned long)fun;
	t->state.rbp = (unsigned long)s_ptr;
	t->state.rsp = (unsigned long)s_ptr;
	t->state.fxsave = FPU_INIT; 			// Initialize predefined FPU_INIT value
	t->status = LWP_LIVE; 				// Thread created, now go live
	
	// New so start out unlinked
	t->lib_one = NULL; 
	t->lib_two = NULL;
	t->sched_one = NULL;
	t->sched_two = NULL;
	t->exited = NULL;

	current_scheduler->admit(t); 					// Add thread to the queue
	printf("I got here!\n");
	return t->tid;
}

// -----lwp_yield-----
void lwp_yield(void) {
	// Save the current context
	thread old_thread = current_thread;
	
	thread new_thread = current_scheduler->next(); // Grab the new thread
	if (new_thread == NULL) {
		fprintf(stderr, "ERROR: no new thread!\n");
		//exit(LWPTERMSTAT(new_thread->status));
		exit(LWPTERMSTAT(old_thread->status));
	}

	// Now current thread is the new thread
	current_thread = new_thread;
	
	printf("Before yield swap_rfile: I got here!\n");
	swap_rfiles(&old_thread->state, &new_thread->state);
}

// -----lwp_start()------
void lwp_start(void) {
	// Start the lwp system. 
	// Converts the calling thread into a LWP
	// Yields to whichever thread the scheduler choose
	scheduler start_scheduler = lwp_get_scheduler(); // get the current scheduler and turn it into an LWP
	DoubleLinkedList_init(&toTerminate);

	printf("start: I got here!\n");
	//Original calling thread - No need to init a stack because it already has one.
	thread original_thread = malloc(sizeof(context));
	original_thread->tid = t_num++;
	original_thread->stack = NULL;
	original_thread->stacksize = 0;
	original_thread->status = LWP_LIVE;

	original_thread->lib_one = NULL;
	original_thread->lib_two = NULL;
	original_thread->sched_one = NULL;
	original_thread->sched_two = NULL;
	original_thread->exited = NULL;
	
	printf("Before swap_rfile: I got here!\n");
	swap_rfiles(&original_thread->state, NULL);
	printf("After swap_rfile: I got here!\n");
	
	// Current thread is now officially the main thread
	current_thread = original_thread; 	
	printf("After swap_rfile: I got here!\n");

	// Add the OG thread to the scheduler (TID should be 1)
	start_scheduler->admit(original_thread);	
	printf("After swap_rfile: I got here!\n");

	// Yield
	lwp_yield();
}

// -----lwp_exit()------
void lwp_exit(int exitval) {
	// Only use lower 8 bits of exitval
	int status = exitval & 0xFF;

	// Mark thread as terminated
	MKTERMSTAT(LWP_TERM, status);

	current_scheduler->remove(current_thread);

	/* Handle any thread blocked in lwp_wait() — if there's a thread sitting on the
 	 * wait queue waiting for something to terminate, you need to take the oldest one
 	 * off the wait queue, associate it with this newly terminated thread, and re-admit
 	 * it to the scheduler with current_scheduler->admit(). 
 	*/
 	DoubleLinkedList_push_back(&toTerminate, current_thread);
	lwp_yield();  	
}

// -----tid2thread-----
thread tid2thread(tid_t t) {
	thread curr_thread = RoundRobin->next();
	if (curr_thread->tid == t) {
		return curr_thread;
	} else {
		return NULL;
	}
}

// -----lwp_set_scheduler-----
void lwp_set_scheduler(scheduler sched) {
	scheduler old_scheduler = current_scheduler;

	// Initialize new schedular, if sched is NULL use round robin default
	if (sched == NULL) {
		//RoundRobin->init();
		sched = RoundRobin;
	} else {
		sched->init();
	}

	// If the old scheduler is already the one we're using, then do nothing
	if (old_scheduler == sched) {
		return;
	}

	// If a scheduler is already active, admit all the threads then shutdown the old scheduler cleanly
	if (old_scheduler != NULL) {
		while(old_scheduler->qlen() > 0) {
			thread t_temp = old_scheduler->next();
			if (t_temp == NULL) {
				break;
			}
	
			old_scheduler->remove(t_temp);
			sched->admit(t_temp);
		}
		
		// Clean up 
		old_scheduler->shutdown();
	}

	// Set the new scheduler to be the current one
	current_scheduler = sched;
	return;
}

// -----lwp_get_scheduler-----
scheduler lwp_get_scheduler(void) {
	return current_scheduler;
}

// -----lwp_gettid-----
tid_t lwp_gettid(void) {
	if (current_thread == NULL) {
		return NO_THREAD;
	}
	else {
		return current_thread->tid;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//volatile int ran = 0;
 //int thread_func(void *args) {
 //       ran = 1;
 //       return 1;
 //}

//nt main(void) {
 //       RoundRobin->init();
 //       tid_t thread_tid = lwp_create(thread_func, NULL);
 //       
 //       thread t = tid2thread(thread_tid);

 //       printf("tid: %lu\n", t->tid);
 //       printf("size %d\n", RoundRobin->qlen());
 //       printf("Stacksize:  %zu\n", t->stacksize);
 //       printf("Base stack: %p\n", (void *)t->stack);
 //       printf("State rsp:  0x%lx\n", t->state.rsp);
 //       printf("State rbp:  0x%lx\n", t->state.rbp);
 //       printf("State rdi:  0x%lx\n", t->state.rdi);
 //       printf("State rsi:  0x%lx\n", t->state.rsi);
 //       printf("Status: %u\n", t->status);

 //       if (t->lib_one == NULL & t->lib_two == NULL & t->sched_one == NULL & t->sched_two == NULL & t->exited == NULL) {
 //       	printf("true\n");
 //       } else {
 //       	printf("false\n");
 //       }

 //       if (thread_tid == NO_THREAD) {
 //       	printf("lwp create failed\n");

 //       }
 //       if (ran == 1) {
 //       	printf("PASS\n");
 //       } else {
 //       	printf("FAILED\n");
 //       }

 //       return 0;
 //}
