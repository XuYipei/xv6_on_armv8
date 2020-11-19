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

uint32_t kmeminitcnt;
struct spinlock kmemlock, kmeminitlock;

void
alloc_init()
{
    struct mcslock locallock;
    // mcsacquire(&kmeminitlock, &locallock);
    acquire(&kmeminitlock);

    if (kmeminitcnt != 0){
        // mcsrelease(&kmeminitlock, &locallock);    
        release(&kmeminitlock);
        return;
    }
    kmeminitcnt = 1;
    
    free_range(end, P2V(PHYSTOP));

    // mcsrelease(&kmeminitlock, &locallock);
    release(&kmeminitlock);
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
    // mcsacquire(&kmemlock, &locallock);
    acquire(&kmemlock);

    r->next = kmem.free_list;
    kmem.free_list = r;

    // mcsrelease(&kmemlock, &locallock);
    release(&kmemlock);
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
        // if ((uint64_t)(p - (char *)vstart) / PGSIZE % 4096 == 0)
        //     cprintf("%llx\n", p);
    }        

}

char *
kalloc()
{
    struct mcslock locallock;
    //mcsacquire(&kmemcslock, &locallock);
    acquire(&kmemcslock);

    if (kmem.free_list == NULL) 
        return(0);
    
    struct mcslock locallock;
    // mcsacquire(&kmemlock, &locallock);
    acquire(&kmemlock);

    struct run *r = kmem.free_list; 
    kmem.free_list = r->next;

    // mcsrelease(&kmemlock, &locallock);
    release(&kmemlock);
    
    return((char *)r);
}

void
check_free_list()
{
    struct mcslock locallock;
    //mcsacquire(&kmemcslock, &locallock);
    acquire(&kmemcslock);

    struct run *p;
    if (!kmem.free_list)
        panic("kmem.free_list is a null pointer!\n");

    struct mcslock locallock;
    // mcsacquire(&kmemlock, &locallock);
    acquire(&kmemlock);

    for (p = kmem.free_list; p; p = p->next) {
        assert((void *)p > (void *)end);
    }

    // mcsrelease(&kmemlock, &locallock);
    release(&kmemlock);
}
