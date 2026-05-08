#include <stdio.h>
#include <stdlib.h>

#include "lwp.h"
#include "DoubleLinkedList.h"

/* Round Robin API Library
 * 
 * static functions to make them private to this file
 * Only RoundRobin is global to other files
 *
 */

// Begin
///////////////////////////////////////////////////////////////////////////////////
static DoubleLinkedList rr_list;

//Initialize Scheduler
static void rr_init(void) {
	// To be called before any threads are admitted to the scheduler
	// Allows the scheduler to set up
	// Allowed to be NULL, don't call it if it is
	DoubleLinkedList_init(&rr_list); // Calling the DLL init for RoundRobin
}

static void rr_shutdown(void) {
	// To be called when lwp lib is done with a schedular to allow cleanups
	// Allowed to be NULL, don't call if it is
	DoubleLinkedList_clear(&rr_list);
}

static void rr_admit(thread new) {
	// Add the passed context to the scheduler's scheduling pool
	if (new == NULL) {
		return;
	}	
	
	DoubleLinkedList_push_back(&rr_list, new);
}

static void rr_remove(thread victim) {	
	// Remove a thread from the pool
	if (victim == NULL) {
		return;
	}

	DoubleLinkedList_remove_thread(&rr_list, victim); // Remove the specified thread from rr_list
}

static thread rr_next(void) {
	// Select a thread to schedule or NULL if there isn't one
	thread t = DoubleLinkedList_get_front(&rr_list); // Get the thread in the front of the list
	if (t == NULL) {
		return NULL;
	}
	
	// Put the current thread to the back line of DLL 	
	DoubleLinkedList_rotate_left(&rr_list);
	return t; 
}

static int rr_qlen(void) {
	// Number of ready threads
	int num_of_threads = DoubleLinkedList_size(&rr_list);
	return num_of_threads;

}


/* NOTE: 
 * MUST MATCH THE ORDER OF LWP.h's scheduler struct
 * struct scheduler = the actual struct type
 * scheduler = a pointer to that struct
 * Private functions to RoundRobin.c
 * RoundRobin_scheduler is an actual struct scheduler
 *	.init     = rr_init
 *	.shutdown = rr_shutdown
 *	...
 *	...
 *
 * After initialization
 * 	RoundRobin_scheduler.init = rr_init
 * 	RoundRobin_scheduler.shutdown = rr_shutdown
 * 	...
 * 	...
 */

static struct scheduler RoundRobin_scheduler = {
	.init 		= rr_init,
	.shutdown 	= rr_shutdown,
	.admit		= rr_admit,
	.remove		= rr_remove,
	.next 		= rr_next,
	.qlen 		= rr_qlen
}; 

// How to call in lwp.c
// 	scheduler CurrentScheduler = RoundRobin
// 	CurrentScheduler->init() which calls rr_init()
// 	CurrentScheduler->shutdown() which calls rr_shutdown()
// 	...
// 	...

// Global to other files
scheduler RoundRobin = &RoundRobin_scheduler; // scheduler is a ptr type that takes ady of struct &RoundRobin_scheduler and store it in RoundRobin
