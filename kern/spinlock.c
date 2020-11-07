#include <stddef.h>
#include "arm.h"
#include "spinlock.h"
#include "console.h"

void 
acquire(struct spinlock *lk)
{
    while (lk->locked || __atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE))
        ;
}

void 
release(struct spinlock *lk)
{
    if (!lk->locked)
        panic("release: not locked\n");
    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
}

struct mcslock zero = {(struct mcslock *)NULL, 0};


void
mcsacquire(struct mcslock *lk, struct mcslock *i){
    i->locked = 0;
    i->next = NULL;
    struct mcslock *tail = __sync_lock_test_and_set(&lk->next, i, __ATOMIC_ACQUIRE);
    if (tail){
        i->locked = 1;
        tail->next = i;
    }
    while (i->locked)
        ;
}

void 
mrelease(struct mcslock *lk, struct mcslock *i){
    if (i->next == NULL){
        if (__sync_val_compare_and_swap(&lk->next, i, NULL)) 
            return;
    }
    while (i->next == 0) 
        ;
    i->next->locked = 0;
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