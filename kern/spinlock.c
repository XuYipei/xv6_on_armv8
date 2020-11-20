#include <stddef.h>
#include "arm.h"
#include "spinlock.h"
#include "console.h"
#include "proc.h"
#include "string.h"

/*
 * Check whether this cpu is holding the lock.
 */
int
holding(struct spinlock *lk)
{
    int hold;
    hold = lk->locked && lk->cpu == thiscpu;
    return hold;
}

void
initlock(struct spinlock *lk, char *name) {
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}

void
acquire(struct spinlock *lk)
{
    if (holding(lk)) {
        panic("acquire: spinlock already held\n");
    }
    while (lk->locked || __atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE))
        ;
    lk->cpu = thiscpu;
}

void
release(struct spinlock *lk)
{
    if (!holding(lk)) {
        panic("release: not locked\n");
    }
    lk->cpu = NULL;
    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
}

<<<<<<< HEAD
=======
struct mcslock zero = {(struct mcslock *)NULL, 0};


>>>>>>> e06d4a5... pgdr
void
mcsacquire(struct mcslock *lk, struct mcslock *i){
    i->locked = 0;
    i->next = NULL;
<<<<<<< HEAD
    struct mcslock *tail = lk->next;
    while (!__atomic_compare_exchange_n(&lk->next, &tail, i, 0, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        tail = lk->next;
=======
    struct mcslock *tail = __sync_lock_test_and_set(&lk->next, i, __ATOMIC_ACQUIRE);
>>>>>>> e06d4a5... pgdr
    if (tail){
        i->locked = 1;
        tail->next = i;
    }
    while (i->locked)
        ;
}

void 
<<<<<<< HEAD
mcsrelease(struct mcslock *lk, struct mcslock *i){
<<<<<<< HEAD
    struct mcslock *x = i;
    if (__atomic_compare_exchange_n(&lk->next, &x, NULL, 0, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        return;
    while (i->next == NULL) 
        ;   
    i->next->locked = 0;
}
=======
=======
mrelease(struct mcslock *lk, struct mcslock *i){
>>>>>>> e62f6d8... clhlock
    if (i->next == NULL){
        if (__sync_val_compare_and_swap(&lk->next, i, NULL)) 
            return;
    }
    while (i->next == 0) 
        ;
    i->next->locked = 0;
<<<<<<< HEAD
}
>>>>>>> e06d4a5... pgdr
=======
    free(i);
}


void
macquire(struct clhlock *lk, struct clhlock *i){
    lk->locked = 1;
    i->prev = __sync_lock_test_and_set(&lk->prev, i, __ATOMIC_ACQUIRE);
    while (__atomic_load_n(&i->prev->locked))
        ;
}

void 
mrelease(struct clhlock *lk, struct clhlock *i){
    if (i->prev){
        free(i->prev);
        i->prev = (struct clhlock *)0;
    }
    __atomic_store_n(i->locked, 0, __ATOMIC_RELEASE);
}
>>>>>>> e62f6d8... clhlock
