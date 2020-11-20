#include <stdint.h>
#include "types.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "console.h"
#include "kalloc.h"
#include "spinlock.h"
#include "arm.h"

extern char end[];

struct run {
    struct run *next;
};

struct {
    struct run *free_list;
} kmem;

uint32_t kmeminitcnt = 0;
<<<<<<< HEAD
struct spinlock kmemcslock, kmeminitlock;
// struct mcslock kmemcslock, kmeminitlock;
=======
//struct spinlock kmemcslock, kmeminitlock;
struct mcslock kmemcslock, kmeminitlock;
>>>>>>> 0c2e05b... dev-lab4

void
alloc_init()
{
    struct mcslock locallock;
<<<<<<< HEAD
    //mcsacquire(&kmeminitlock, &locallock);
    acquire(&kmeminitlock);

    if (kmeminitcnt != 0){
        //mcsrelease(&kmeminitlock, &locallock);
        release(&kmeminitlock);
=======
    //cprintf("acquire %llx\n", &locallock);
    mcsacquire(&kmeminitlock, &locallock);
    //acquire(&kmeminitlock);

    if (kmeminitcnt != 0){
        mcsrelease(&kmeminitlock, &locallock);
        //release(&kmeminitlock);
>>>>>>> 0c2e05b... dev-lab4
        return;
    }
    kmeminitcnt = 1;
    
    free_range(end, P2V(PHYSTOP));

<<<<<<< HEAD
    //mcsrelease(&kmeminitlock, &locallock);
    release(&kmeminitlock);
=======
    mcsrelease(&kmeminitlock, &locallock);
    //cprintf("%llx release %llx %llx %x\n", &locallock, locallock.next, kmeminitlock.next, locallock.next->locked);
    //release(&kmeminitlock);
>>>>>>> 0c2e05b... dev-lab4
}

void
kfree(char *v)
{
    struct run *r;

    if ((uint64_t)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
        panic("kfree\n");

    memset(v, 1, PGSIZE);
    r = (struct run *)v;

    struct mcslock locallock;
<<<<<<< HEAD
    //mcsacquire(&kmemcslock, &locallock);
    acquire(&kmemcslock);
=======
    mcsacquire(&kmemcslock, &locallock);
    //acquire(&kmemcslock);
>>>>>>> 0c2e05b... dev-lab4

    r->next = kmem.free_list;
    kmem.free_list = r;

<<<<<<< HEAD
    //mcsrelease(&kmemcslock, &locallock);
    release(&kmemcslock);
=======
    mcsrelease(&kmemcslock, &locallock);
    //release(&kmemcslock);
>>>>>>> 0c2e05b... dev-lab4
}

void
free_range(void *vstart, void *vend)
{
    //cprintf("%llx\n", (uint64_t)vstart);
    //cprintf("%llx\n", (uint64_t)vend);

    char *p;
    p = ROUNDUP((char *)vstart, PGSIZE);
    for (; p + PGSIZE <= (char *)vend; p += PGSIZE){
        kfree(p);
    }        

}

char *
kalloc()
{
    struct mcslock locallock;
<<<<<<< HEAD
    //mcsacquire(&kmemcslock, &locallock);
    acquire(&kmemcslock);
=======
    mcsacquire(&kmemcslock, &locallock);
    //acquire(&kmemcslock);
>>>>>>> 0c2e05b... dev-lab4

    if (kmem.free_list == NULL) 
        return(0);

    struct run *r = kmem.free_list; 
    kmem.free_list = r->next;

<<<<<<< HEAD
    //mcsrelease(&kmemcslock, &locallock);
    release(&kmemcslock);
=======
    //cprintf("%llx %llx\n", (uint64_t)locallock.next);
    mcsrelease(&kmemcslock, &locallock);
    //release(&kmemcslock);
>>>>>>> 0c2e05b... dev-lab4
    
    return((char *)r);
}

void
check_free_list()
{
    struct mcslock locallock;
<<<<<<< HEAD
    //mcsacquire(&kmemcslock, &locallock);
    acquire(&kmemcslock);
=======
    mcsacquire(&kmemcslock, &locallock);
    //acquire(&kmemcslock);
>>>>>>> 0c2e05b... dev-lab4

    struct run *p;
    if (!kmem.free_list)
        panic("kmem.free_list is a null pointer!\n");

    for (p = kmem.free_list; p; p = p->next) {
        assert((void *)p > (void *)end);
    }

<<<<<<< HEAD
    //mcsrelease(&kmemcslock, &locallock);
    release(&kmemcslock);
}
=======
    mcsrelease(&kmemcslock, &locallock);
    //release(&kmemcslock);
}
>>>>>>> 0c2e05b... dev-lab4
