#ifndef INC_SPINLOCK_H
#define INC_SPINLOCK_H


struct spinlock {
    volatile int locked;
};
void acquire(struct spinlock *);
void release(struct spinlock *);


struct mcslock {
    struct mcslock *next;
    volatile int locked;         
};
void mcsacquire(struct mcslock *, struct mcslock *);
void mcsrelease(struct mcslock *, struct mcslock *);

#endif
