#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_CMD_LEN 256
#define MAX_ARGS 11
typedef struct
{
	char *process[MAX_ARGS + 1]; // stores each process of size 256, including arguments up to 10
	pid_t pid;					 // Child process ID
								 // int valid;					 // 1 = active, 0 empty
								 // int finished;				 // 1 = exited, 0 = running
} QueueEntry;

typedef struct
{
	QueueEntry *entries; // Dynamically allocated array
	int frontQueue;		 // first element index
	int backQueue;		 // last element index
	int sizeQueue;		 // max queue capacity
	int countQueue;		 // current number of elements
} CircularQueue;

int CircularQueue_init(CircularQueue *queue, int sizeQueue);
int CircularQueue_insert(CircularQueue *queue, char **process);
int CircularQueue_remove(CircularQueue *queue);
QueueEntry *CircularQueue_get(CircularQueue *queue);

// Support functions
int CircularQueue_is_full(CircularQueue *queue);
int CircularQueue_is_empty(CircularQueue *queue);
int CircularQueue_clear(CircularQueue *queue);
int CircularQueue_free(CircularQueue *queue);

#endif
