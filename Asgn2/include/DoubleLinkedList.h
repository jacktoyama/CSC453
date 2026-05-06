#ifndef DOUBLELINKEDLIST_H
#define DOUBLELINKEDLIST_H

#include "lwp.h"
#include <stdbool.h>
/* Double Linke List for thread objects
 *
 * Uses:
 * 	thread->sched_one as next
 * 	thread->sched_two as prev
 *
 * The list itself does not allocate or free threads
 *
 */

typedef struct DoubleLinkedList {
	thread head;
	thread tail;
	int size;
} DoubleLinkedList;

// Unlink
//void DoubleLinkedList_unlink(thread t);

// DLL Operations
void DoubleLinkedList_init(DoubleLinkedList *list);
int DoubleLinkedList_size(DoubleLinkedList *list);
bool DoubleLinkedList_is_empty(DoubleLinkedList *list);

// Get front and back
thread DoubleLinkedList_get_front(DoubleLinkedList *list);
thread DoubleLinkedList_get_back(DoubleLinkedList *list);

// Insert
void DoubleLinkedList_push_front(DoubleLinkedList *list, thread t);
void DoubleLinkedList_push_back(DoubleLinkedList *list, thread t);

// Remove
thread DoubleLinkedList_remove_front(DoubleLinkedList *list);
thread DoubleLinkedList_remove_back(DoubleLinkedList *list);
void DoubleLinkedList_remove_thread(DoubleLinkedList *list, thread t);

// Util
void DoubleLinkedList_rotate_left(DoubleLinkedList *list); // Move head to tail
void DoubleLinkedList_clear(DoubleLinkedList *list); // Unlink all nodes, not free

// Debug
int DoubleLinkedList_contains(DoubleLinkedList *list, thread t);

#endif

