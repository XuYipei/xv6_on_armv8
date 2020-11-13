#include <stddef.h>
#include <stdlib.h>
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
    struct mcslock *tail = lk->next;
    while (!__atomic_compare_exchange_n(&lk->next, &tail, i, 0, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        tail = lk->next;
    if (tail){
        i->locked = 1;
        tail->next = i;
    }
    while (i->locked)
        ;
}

void 
mcsrelease(struct mcslock *lk, struct mcslock *i){
    struct mcslock *x = i;
    if (__atomic_compare_exchange_n(&lk->next, &x, NULL, 0, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        return;
    while (i->next == NULL) 
        ;   
    i->next->locked = 0;
}
