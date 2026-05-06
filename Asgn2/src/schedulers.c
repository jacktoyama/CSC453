#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>
#include "RoundRobin.h"
#include "DoubleLinkedList.h"


int thread1(void *arg) {
    printf("Thread 1 running\n");
    return 1;
}

int thread2(void *arg) {
    printf("Thread 2 running\n");
    return 2;
}

int thread_func(void *arg) {
    int x = (int)(long)arg;
    printf("Value: %d\n", x);
    return x;
}

// Main function to call the lwp library to run functions
int main(void) {
	// Testing
	int val = 42;

	// Start the scheduler
	//lwp_set_scheduler(RoundRobin); // RR is the main scheduler 	
	lwp_create(thread1, NULL); // take thread1 function with NULL arg
	lwp_create(thread2, NULL); // take thread2 function with NULL arg
	lwp_create(thread_func, (void *)42);
	lwp_start(); // start the LWP system


	int status;
	while (1) {
		tid_t tid = lwp_wait(&status);
		if (tid == NO_THREAD) {
			break;
		}

		// Else
		printf("Thread %lu exited with %d\n", tid, status);
	}
	return 0;
}

//	RoundRobin->init();
//	tid_t thread_tid = lwp_create(thread_func, NULL);
//	
//	thread t = tid2thread(thread_tid);
//
//	printf("tid: %lu\n", t->tid);
//	printf("size %d\n", RoundRobin->qlen());
//	printf("Stacksize:  %zu\n", t->stacksize);
//	printf("Base stack: %p\n", (void *)t->stack);
//	printf("State rsp:  0x%lx\n", t->state.rsp);
//	printf("State rbp:  0x%lx\n", t->state.rbp);
//	printf("State rdi:  0x%lx\n", t->state.rdi);
//	printf("State rsi:  0x%lx\n", t->state.rsi);
//	printf("Status: %u\n", t->status);
//
//	if (t->lib_one == NULL & t->lib_two == NULL & t->sched_one == NULL & t->sched_two == NULL & t->exited == NULL) {
//		printf("true\n");
//	} else {
//		printf("false\n");
//	}
//
//	if (thread_tid == NO_THREAD) {
//		printf("lwp create failed\n");
//
//	}
//	if (ran == 1) {
//		printf("PASS\n");
//	} else {
//		printf("FAILED\n");
//	}


