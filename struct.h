
typedef struct scheduler {
    void (*init)(void); 		/* initialize any structure */
    void (*shutdown)(void);     	/* tear down any structure */
    void (*admit)(thread new);  	/* add a thread to the pool*/
    void (*remove)(thread victim); 	/* remove a thread from the pool */
    void (*next)(void); 		/* select a thread to schedule*/
    int (*glen)(void); 			/* number of ready threads */
} *scheduler;
