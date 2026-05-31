#define _GNU_SOURCE
#define MAX_THREADS 65536
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include <sys/resource.h>
#include "../include/RoundRobin.h"
#include "../include/lwp.h"
#include "../include/DoubleLinkedList.h"

// Global variables
static thread current_thread = NULL;
static scheduler current_scheduler = NULL;
unsigned long t_num = 1;
static thread TrackThreadsList[MAX_THREADS]; //  list to track threads for tid2thread


static DoubleLinkedList toTerminate;   // threads that terminated
static DoubleLinkedList toWait;	// threads blocked in lwp_wait()

// Suport function
static void TrackThread(thread t) {
	if (t->tid < MAX_THREADS) {
		TrackThreadsList[t->tid] = t;
	}
}


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
	// Default scheduling is RoundRobin
	if (current_scheduler == NULL) {
		current_scheduler = RoundRobin;
		if (current_scheduler->init != NULL) {
			current_scheduler->init();
		}
	}


	// Find stack size	
	struct rlimit lim;
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size == -1) {
		fprintf(stderr, "Error: sysconf failed!\n");
	//} else {
	//	printf("System page size: %ld bytes\n", page_size);
	}

	// Grab stack size resource soft limit
	long soft_limit;
	if (getrlimit(RLIMIT_STACK, &lim) == 0) {
		soft_limit = (long)lim.rlim_cur; // Get soft litmit of stack		
		if (soft_limit == RLIM_INFINITY) {
			soft_limit = 8388608; // 8MB bytes	
		}
		//printf("Soft limit: %ld\n", soft_limit);
	//} else {
	//	fprintf(stderr, "Error: getrlimit failed!\n");
	}

	// Round page size in case other machines don't have the exact 8Mb bytes
	long rounded_stacksize = ((soft_limit + page_size - 1) / page_size ) * page_size;
		
	// NOTE: mmap returns the start of the mapped region; aka lowest addy/base/bottom of stack).
	// Stack grows downward (high address to low address)
	
	// Create a thread object
	thread t = malloc(sizeof(context)); // gets access to context struct
	if (t == NULL) {
		return NO_THREAD;
	}
	
	// Create a stack frame for it
	t->tid = t_num++;
	t->stack = mmap(NULL, rounded_stacksize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);	
	if (t->stack == MAP_FAILED) {
		//fprintf(stderr, "ERROR: mmap failed!\n");
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
	
	// Fake return after wrapper
	s_ptr = s_ptr - 1; // 0x80100 - 8 = 0x80ff8 Move 8bytes down from tos
	*s_ptr = 0; // Set value at 0x80ff8 = 0 
	
	// Address of lwp_wrap
    s_ptr = s_ptr - 1;
	*s_ptr = (unsigned long)lwp_wrap; // store the adddress of lwp_wrap at 0x800ff0
	
	// fake old rbp <- saved rbp/rsp point here
	s_ptr = s_ptr - 1;
	*s_ptr = 0;

	t->state.rsi = (unsigned long)args;
	t->state.rdi = (unsigned long)fun;
	t->state.rbp = (unsigned long)s_ptr;
	t->state.rsp = (unsigned long)s_ptr;
	t->state.fxsave = FPU_INIT; 			// Initialize predefined FPU_INIT value
	t->status =  MKTERMSTAT(LWP_LIVE, 0); 				// Thread created, now go live
	
	// New so start out unlinked
	t->lib_one = NULL; 
	t->lib_two = NULL;
	t->sched_one = NULL;
	t->sched_two = NULL;
	t->exited = NULL;

	TrackThread(t);
	current_scheduler->admit(t); 			// Add thread to the queue
	return t->tid;
}


// -----lwp_exit()------
void lwp_exit(int exitval) {
	thread exited_thread = current_thread;

	// Only use lower 8 bits of exitval for exit code
	int status = exitval & 0xFF;

	// Mark thread as terminated
	exited_thread->status = MKTERMSTAT(LWP_TERM, status);
	
	// Removeq from current scheduler 
	current_scheduler->remove(exited_thread); // dead threads should not run again

	// if another thread is waiting for it, wake it up
	if (!DoubleLinkedList_is_empty(&toWait)) { // if not empty
		thread thread2wait = DoubleLinkedList_remove_front(&toWait); // Grab the first in list
		thread2wait->exited = exited_thread; // pass this dead thread
		current_scheduler->admit(thread2wait);
	} else {
		// No processes waiting, so terminate 
		DoubleLinkedList_push_back(&toTerminate, exited_thread);
	}

	// Switch to another thread
	lwp_yield();  	
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

// -----lwp_yield-----
void lwp_yield(void) {
	// Save the current context
	thread old_thread = current_thread;
	//printf("I was here\n");	
	thread new_thread = current_scheduler->next(); // Grab the new thread
	
	// No new thread, so exit with the old thread's status
	if (new_thread == NULL) {
		//fprintf(stderr, "ERROR: no new thread!\n");
		//xit(LWPTERMSTAT(new_thread->status));
		//printf("I was here 2\n");
		//printf("exit code %d\n", LWPTERMSTAT(old_thread->status));	
		exit(LWPTERMSTAT(old_thread->status));
	}

	if (new_thread == old_thread) {
		return;
	}

	//printf("I was here 3\n");	
	// Now current thread is the new thread
	current_thread = new_thread;
	
	swap_rfiles(&old_thread->state, &new_thread->state);
}

// -----lwp_start()------
void lwp_start(void) {
	// if scheduler is not set, intialize it
	if (current_scheduler == NULL) {
		current_scheduler = RoundRobin;
		if (current_scheduler->init != NULL) {
			current_scheduler->init();
		}
	}

	// Start the lwp system. 
	// Converts the calling thread into a LWP
	// Yields to whichever thread the scheduler choose
	scheduler start_scheduler = lwp_get_scheduler(); // get the current scheduler and turn it into an LWP

	// Initialize queues used for holding and waiting forterminated threads
	DoubleLinkedList_init(&toTerminate);
	DoubleLinkedList_init(&toWait);

	// Creating contexts for original calling thread - No need to init a stack because it already has one.
	thread original_thread = malloc(sizeof(context));
	original_thread->tid = t_num++;
	original_thread->stack = NULL;
	original_thread->stacksize = 0;
	original_thread->status = MKTERMSTAT(LWP_LIVE, 0);

	original_thread->lib_one = NULL;
	original_thread->lib_two = NULL;
	original_thread->sched_one = NULL;
	original_thread->sched_two = NULL;
	original_thread->exited = NULL;
	
	swap_rfiles(&original_thread->state, NULL);
	
	// Current thread is now officially the main thread
	current_thread = original_thread; 	
	
	// Add the OG thread to the scheduler (TID should be 1)
	start_scheduler->admit(original_thread);	

	// Add thread to track 	
	TrackThread(original_thread);	

	// Yield
	lwp_yield();
}

// -----lwp_wait-----
tid_t lwp_wait(int *status) {
	thread terminatedThread; // Initiate a thread variable

	// SITUATION #1
	// If there's a terminated thread / non-blocking
	if (!DoubleLinkedList_is_empty(&toTerminate)) {
		terminatedThread = DoubleLinkedList_remove_front(&toTerminate); // grab the first terminated thread
	
		// Reap the oldest one in queue
		if (status != NULL) {
			*status = terminatedThread->status;
		}

		// Grab the terminated thread's tid
		tid_t tid2reap = terminatedThread->tid;
		TrackThreadsList[terminatedThread->tid] = NULL;
		if (terminatedThread->stack != NULL) {
			munmap(terminatedThread->stack, terminatedThread->stacksize); // Deallocate stack
		}

		// Prevent memory leaks
		free(terminatedThread);
		return tid2reap; // return status of a fully executed terminated thread
	}	

	// SITUATION #2 (Exception to Situation #3)
	// If no active threads to terminate, then return no thread
	if (current_scheduler->qlen() <= 1) {
		return NO_THREAD;
	}
	
	// SITUATION #2	
	// If no active threads to terminate, then return no thread
	if (current_scheduler->qlen() <= 1) {
		return NO_THREAD;
	}

	// SITUATION #3
	// ----- Wait for some future lwp_exit() hands it a dead thread
	// No terminated threads, so caller of lwp_wait() block
	thread waitForThread = current_thread; 			// Set current thread = waitForThread variable
	current_scheduler->remove(waitForThread); 		// Remove this waitForThread from the current scheduler
	DoubleLinkedList_push_back(&toWait, waitForThread); 	// Put waitForThread into toWait DLL queue
	lwp_yield(); 						// Let another thread calls it



	// When resumed, lwp_exit() sets threadToWait->exited
	terminatedThread = waitForThread->exited; 		// terminatedThread now the exited thread
	waitForThread->exited = NULL; 				// reset the exited thread to NULL now that we have it

	// No thread then just exit 
	if (terminatedThread == NULL) {
		return NO_THREAD;
	}

	// Reap the oldest one
	if (status != NULL) {
		*status = terminatedThread->status;
	}

	// Grab the terminated thread's tid
	tid_t tid2reap = terminatedThread->tid;
	TrackThreadsList[terminatedThread->tid] = NULL;
	if (terminatedThread->stack != NULL) {
		munmap(terminatedThread->stack, terminatedThread->stacksize); // Deallocate stack
	}
	
	// Free it
	free(terminatedThread);
	return tid2reap;
}


// -----tid2thread-----
thread tid2thread(tid_t t) {
	if (t != NO_THREAD || t < MAX_THREADS) {
		return TrackThreadsList[t];
	
	}
	return NULL;
}

// -----lwp_set_scheduler-----
void lwp_set_scheduler(scheduler sched) {
    scheduler old_scheduler = current_scheduler;

    // Default to RoundRobin if NULL
    if (sched == NULL) {
        sched = RoundRobin;
    }

    // If already using this scheduler, do nothing
    if (old_scheduler == sched) {
        return;
    }

    // Initialize the new scheduler ONLY now (not before the same-check)
    if (sched->init != NULL) {
        sched->init();
    }

    // Transfer all threads from old scheduler to new one
    if (old_scheduler != NULL) {
    	thread t_temp;
    	while ((t_temp = old_scheduler->next()) != NULL) {
        	old_scheduler->remove(t_temp);
        	sched->admit(t_temp);
    	}

    	if (old_scheduler->shutdown != NULL) {
        	old_scheduler->shutdown();
    	}
	}

    current_scheduler = sched;
}

// -----lwp_get_scheduler-----
scheduler lwp_get_scheduler(void) {
	return current_scheduler;
}

