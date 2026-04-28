#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>
#include "DoubleLinkedList.h"
#include "RoundRobin.h"

int main(void) {
    RoundRobin->init();

    thread t1 = malloc(sizeof(context));
    thread t2 = malloc(sizeof(context));
    thread t3 = malloc(sizeof(context));

    t1->tid = 1;
    t2->tid = 2;
    t3->tid = 3;
    if (t1 == NULL || t2 == NULL || t3 == NULL) {
        perror("malloc");
        return 1;
    }

    t1->sched_one = NULL;
    t1->sched_two = NULL;
    t2->sched_one = NULL;
    t2->sched_two = NULL;
    t3->sched_one = NULL;
    t3->sched_two = NULL;

    printf("Size: %d\n", RoundRobin->qlen());
    
    RoundRobin->admit(t1);
    printf("Size: %d\n", RoundRobin->qlen());

    RoundRobin->admit(t2);
    printf("Size: %d\n", RoundRobin->qlen());

    RoundRobin->admit(t3);
    printf("Size: %d\n", RoundRobin->qlen());

    thread next = RoundRobin->next();
    thread next2 = RoundRobin->next();
    printf("Next thread: tid=%lu\n", next2->tid);
    
    //RoundRobin->remove(t2);
    //RoundRobin->remove(t1);
    //RoundRobin->remove(t3);
    RoundRobin->shutdown();
    printf("Size: %d\n", RoundRobin->qlen());
    



    free(t1);
    free(t2);
    free(t3);
    return 0;
}

