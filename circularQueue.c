// ----- Circular Queue Library -----
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "circularQueue.h"

int CircularQueue_init(CircularQueue *queue, int size)
{
	queue->entries = malloc(sizeof(QueueEntry) * size);
	if (!queue->entries)
	{
		return -1;
	}

	queue->sizeQueue = size;
	queue->frontQueue = 0;
	queue->backQueue = -1;
	queue->countQueue = 0;

	for (int i = 0; i < size; i++)
	{
		queue->entries[i].pid = -1;

		for (int j = 0; j < MAX_ARGS + 1; j++)
		{
			queue->entries[i].process[j] = NULL;
		}
	}
	return 0;
}

int CircularQueue_insert(CircularQueue *queue, char **process)
{
	if (CircularQueue_is_full(queue))
	{
		return -1;
	}

	// Move the line forward
	queue->backQueue = (queue->backQueue + 1) % queue->sizeQueue;

	// Init prt to backQueue for insertion
	QueueEntry *entry = &queue->entries[queue->backQueue];

	entry->pid = -1;

	// Copy the process into the queue
	int i;
	for (i = 0; process[i] != NULL && i < MAX_ARGS; i++)
	{
		entry->process[i] = strdup(process[i]);
		if (!entry->process[i])
		{
			return -1;
		}
	}
	entry->process[i] = NULL;
	queue->countQueue++;
	return 0;
}

QueueEntry *CircularQueue_get(CircularQueue *queue)
{
	if (CircularQueue_is_empty(queue))
	{
		return NULL;
	}
	return &queue->entries[queue->frontQueue];
}

int CircularQueue_remove(CircularQueue *queue)
{
	if (CircularQueue_is_empty(queue))
	{
		return -1;
	}

	QueueEntry *entry = &queue->entries[queue->frontQueue];

	int i = 0;
	while (entry->process[i] != NULL)
	{
		free(entry->process[i]);
		entry->process[i] = NULL;
		i++;
	}

	// entry->valid = 0;
	// entry->finished = 0;
	entry->pid = -1;

	queue->frontQueue = (queue->frontQueue + 1) % queue->sizeQueue;
	queue->countQueue--;
	return 0;
}

int CircularQueue_is_full(CircularQueue *queue)
{
	return queue->countQueue == queue->sizeQueue;
}

int CircularQueue_is_empty(CircularQueue *queue)
{
	return queue->countQueue == 0;
}

int CircularQueue_free(CircularQueue *queue)
{
	if (queue->entries != NULL)
	{
		for (int i = 0; i < queue->sizeQueue; i++)
		{
			int j = 0;
			while (queue->entries[i].process[j] != NULL)
			{
				free(queue->entries[i].process[j]);
				queue->entries[i].process[j] = NULL;
				j++;
			}
		}
		free(queue->entries);
		queue->entries = NULL;
	}

	queue->frontQueue = 0;
	queue->backQueue = -1;
	queue->sizeQueue = 0;
	queue->countQueue = 0;

	return 0;
}
