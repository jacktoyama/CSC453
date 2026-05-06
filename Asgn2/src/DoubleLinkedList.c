#include <stddef.h>
#include <stdbool.h>
#include "DoubleLinkedList.h"

/* Remainder
 * sched_one = next
 * sched_two = prev
 */

// Unlink
static void DoubleLinkedList_unlink(thread t) {
	if (t == NULL) {
   		return;
	}
	t->sched_one = NULL;
    	t->sched_two = NULL;
}

// Init
void DoubleLinkedList_init(DoubleLinkedList *list) {
	if (list == NULL) {
		return;
	}
	
	// Initialize DLL
	list->head = NULL;
	list->tail = NULL;
 	list->size = 0;
}

int DoubleLinkedList_size(DoubleLinkedList *list) {
	if (list == NULL) {
		return 0;
	} else {
		return list->size;
	}
}

bool DoubleLinkedList_is_empty(DoubleLinkedList *list) {	
	if (DoubleLinkedList_size(list) == 0) {
		return true;
	} else {
		return false;	
	}
}

// Get front and back
thread DoubleLinkedList_get_front(DoubleLinkedList *list) {
	if (list->head == NULL) {
		return NULL;
	} else {
		return list->head;
	}
}

thread DoubleLinkedList_get_back(DoubleLinkedList *list) {
	if (list->tail == NULL) {
		return NULL;
	} else {
		return list->tail;
	}
}

// Insert 
void DoubleLinkedList_push_front(DoubleLinkedList *list, thread t) {
	if (list == NULL || t == NULL) {
		return;
	}

	t->sched_one = list->head; // Next one of new t is old head
	t->sched_two = NULL; // Prev one of new t is NULL

	if (list->head != NULL) {
		list->head->sched_two = t; // Prev of the old head is now t
	} else {
		// List was empty
		list->tail = t;
	}

	list->head = t;
	list->size++;
}

void DoubleLinkedList_push_back(DoubleLinkedList *list, thread t) {
    if (list == NULL || t == NULL) return;

    t->sched_one = NULL; // Next of new t is NULL
    t->sched_two = list->tail; // Prev one of new t is old tail

    if (list->tail != NULL) { 
        list->tail->sched_one = t; // Next one of old tail is new t 
    } else {
        // list empty
        list->head = t; // [t0] only node
    }

    list->tail = t; // current tail is now new t
    list->size++;
}

// Pop
thread DoubleLinkedList_remove_front(DoubleLinkedList *list) {
    thread t;

    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    t = list->head; // Get the head t
    list->head = t->sched_one; // Current head is now the next one of old head

    if (list->head != NULL) {
        list->head->sched_two = NULL; // Prev of new head is NULL;
    } else {
        // list is empty
        list->tail = NULL; // [] nothing in after pop
    }

    DoubleLinkedList_unlink(t);
    list->size--;
    return t;
}

thread DoubleLinkedList_remove_back(DoubleLinkedList *list) {
    thread t;

    if (list == NULL || list->tail == NULL) {
        return NULL;
    }

    t = list->tail;
    list->tail = t->sched_two; // Current tail is now old tail's prev one

    if (list->tail != NULL) {
        list->tail->sched_one = NULL; // next one of current tail is NULL
    } else {
        // list empty 
        list->head = NULL;
    }

    DoubleLinkedList_unlink(t);
    list->size--;
    return t;
}

// Case A: t0 <-> t1 <-> t2
void DoubleLinkedList_remove_thread(DoubleLinkedList *list, thread t) {
	if (list == NULL || t == NULL || list->size == 0) {
		return;		
	}

	//  Head part
	if (t->sched_two != NULL) { // If prev one of t is not NULL
		t->sched_two->sched_one = t->sched_one; // Next of prev of t is next of t
	} else {
		// Case: t1 <-> t2
		// To remove: t1 
		// Remove head
		if (list->head == t) {
			list->head = t->sched_one; // New head is t2
		} else {
			return; // t not in list
		}
	}

	// Tail part
	if (t->sched_one != NULL) {
		t->sched_one->sched_two = t->sched_two; // Prev of next is prev of old t
	} else {
		// Case:  t1 <-> t2
		// To remove: t2
		// Remove tail
		if (list->tail == t) {
			list->tail = t->sched_two;
		} else {
			return; // t not in list
		}
	}

	DoubleLinkedList_unlink(t);
	list->size--;
}

// Rotate DLL
// Put the current t back to the line of the DLL
void DoubleLinkedList_rotate_left(DoubleLinkedList *list) {
	thread t;

	if (list == NULL || list->size == 0) {
		return;
	}

	t = DoubleLinkedList_remove_front(list);
	DoubleLinkedList_push_back(list, t);
}

// Clear DLL
void DoubleLinkedList_clear(DoubleLinkedList *list) {
	if (list == NULL) {
		return;
	}	
	
	thread current_t;
	thread next_t;

	// t0 <-> t1 <-> t2 <-> t3
	current_t = list->head;
	while (current_t != NULL) {
		next_t = current_t->sched_one;
		current_t->sched_one = NULL;
		current_t->sched_two = NULL;
		current_t = next_t;
		
	}

	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

// Contains
int DoubleLinkedList_contains(DoubleLinkedList *list, thread t) {
	if (list == NULL || t == NULL) {
		return 0;
	}

	thread current_t;
	current_t = list->head;

	while (current_t != NULL) {
		if (current_t == t) {
			return 1;
		}
		current_t = current_t->sched_one;
	}	

	return 0;
}



