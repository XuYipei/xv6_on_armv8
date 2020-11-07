#ifndef INC_SPINLOCK_H
#define INC_SPINLOCK_H

#define MLOCK clhlock

struct spinlock {
    volatile int locked;
};
void acquire(struct spinlock *);
void release(struct spinlock *);


struct mcslock {
    struct mcslock *next;
    volatile int locked;         
};
void macquire(struct mcslock *, struct mcslock *);
void mrelease(struct mcslock *, struct mcslock *);

struct clhlock
{
    struct clhlock *prev;
    volatile int locked;
};
void macquire(struct clhlock *, struct clhlock *);
void mrelease(struct clhlock *, struct clhlock *);

#endif
