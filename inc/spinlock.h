#ifndef INC_SPINLOCK_H
#define INC_SPINLOCK_H

<<<<<<< HEAD
<<<<<<< HEAD
#include "proc.h"
=======
#define MLOCK clhlock
>>>>>>> e62f6d8... clhlock
=======
>>>>>>> 0c2e05b... dev-lab4

struct spinlock {
    volatile int locked;
    
    /* For debugging: */
    char        *name;      /* Name of lock. */
    struct cpu  *cpu;       /* The cpu holding the lock. */
};

int holding(struct spinlock *);
void acquire(struct spinlock *);
void release(struct spinlock *);
void initlock(struct spinlock *, char *);


struct mcslock {
    struct mcslock *next;
    volatile int locked;         
};
void mcsacquire(struct mcslock *, struct mcslock *);
void mcsrelease(struct mcslock *, struct mcslock *);


struct mcslock {
    struct mcslock *next;
    volatile int locked;         
};
void mcsacquire(struct mcslock *, struct mcslock *);
void mcsrelease(struct mcslock *, struct mcslock *);

#endif
