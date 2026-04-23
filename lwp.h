#ifndef LWPH
#define LWPH
#include <sys/types.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if defined(__x86_64)
#include <fp.h>
typedef struct __attribute__((aligned(16)))__attribute__((packed))
registers {
    unsigned long rax;                  /* The 16 architecturally-visible registers */
    unsigned long rbx;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;
    unsigned long rbp;
    unsigned long rsp;
    unsigned long r8;
    unsigned long r9;
    unsigned long r10;
    unsigned long r11;
    unsigned long r12;
    unsigned long r13;
    unsigned long r14;
    unsigned long r15;
    struct fxsave fxsave;               /* space to save floating point state */
} rfile;
#else
    #error "This only works on x86_64 for now"
#endif

typedef unsigned long tid_t;
#define NO_THREAD 0                     /* an always invalid thread ID */

typedef struct threadinfo_st *thread;
typedef struct threadinfo_st {
    tid_t tid;                          /* lightweight process ID */
    unsigned long *stack;               /* Base of allocated stack */
    size_t stacksize;                   /* Size of allocated stack */
    rfile state;                        /* saved registers */
    unsigned int status;                /* exited? exit status? */
    thread lib_one; 			/* Two pointers reserved */
    thread lib_two;
    thread sched_one;
    thread sched_two;
    thread exited;
} context;

typdef int (*lwpfun)(void *);

typedef struct scheduler {
    void (*init)(void); 		/* initialize any structure */
    void (*shutdown)(void);     	/* tear down any structure */
    void (*admit)(thread new);  	/* add a thread to the pool*/
    void (*remove)(thread victim); 	/* remove a thread from the pool */
    void (*next)(void); 		/* select a thread to schedule*/
    int (*glen)(void); 			/* number of ready threads */
} *scheduler;
