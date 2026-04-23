//#include "lwp.h"
#include "struct.h"
#include <stdid.h>
#include <stdlib.h>
#include "circularQueue.h"

// Function to call scheduler
void run_fcfs_scheduler(scheduler *s_scheduler);

// Function Prototypes
void fcfs_init(void);
void fcfs_shutdown(void);
void fcfs_admit(thread new);
void fcfs_remove(thread victim);
void fcfs_next(void);
void fcfs_glen(void);

// Initializing the struct with function addresses
scheduler fcfc_scheduler = {
	.init = fcfs_init,
 	.shutdown = fcfs_shutdown,
	.admit = fcfs_admit,
	.remove = fcfs_remove,
	.next = fcfs_next,
	.glen = fcfs_glen
};

//CircularQueue queue;

//void fcfs_init(void) {
//	CircularQueue_init(&queue, 10);
//}

void fcfs_admit(thread new) {
	if !(CircularQueue_is_full(&queue)) {
		CircularQueue_insert(&queue, new);
	}
	return NULL;
}

void fcfs_remove(thread victim) {
	for (int i = 0; i < queue.countQueue; i++) {
		Queueentry *p = &queue.entries[i];
		if (p->process[i] == victim) {
			CircularQueue_remove(&queue);
		}		
	}

}

int main() {
	CircularQueue queue;
	CircularQueue_init(&queue, 5);

	// admit threads
	thread t1= 1;
	thread_t2 = 2;
	fcfs_admit(t1);
	fcfs_admit(t2);

	printf("%d", CircularQueue_get(&queue));

}


