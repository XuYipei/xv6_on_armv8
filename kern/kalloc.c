#include <stdint.h>
#include "types.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "console.h"
#include "kalloc.h"
#include "spinlock.h"

extern char end[];

struct run {
    struct run *next;
};

struct {
    struct run *free_list;
} kmem;

uint32_t kmeminitcnt;
// struct spinlock kmemlock, kmeminitlock;
struct MLOCK kmemlock, kmeminitlock;

void
alloc_init()
{
    struct MLOCK * locallock = (struct MLOCK *)malloc(sizeof(struct MLOCK));
    macquire(&kmeminitlock, locallock);
    // acquire(&kmeminitlock);

    if (kmeminitcnt != 0){
        mrelease(&kmeminitlock, locallock);
        // release(&kmeminitlock);
        return;
    }
    kmeminitcnt = 1;
    
    free_range(end, P2V(PHYSTOP));

    mrelease(&kmeminitlock, locallock);
    // release(&kmeminitlock);
}

void
kfree(char *v)
{
    struct run *r;

    if ((uint64_t)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
        panic("kfree\n");

    memset(v, 1, PGSIZE);
    r = (struct run *)v;

    struct MLOCK * locallock = (struct MLOCK *)malloc(sizeof(struct MLOCK));
    macquire(&kmemlock, locallock);
    // acquire(&kmemlock);

    r->next = kmem.free_list;
    kmem.free_list = r;

    mrelease(&kmemlock, locallock);
    // release(&kmemlock);
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
    if (kmem.free_list == NULL) 
        return(0);
    
    struct MLOCK * locallock = (struct MLOCK *)malloc(sizeof(struct MLOCK));
    macquire(&kmemlock, locallock);
    // acquire(&kmemlock);

    struct run *r = kmem.free_list; 
    kmem.free_list = r->next;

    mrelease(&kmemlock, locallock);
    // release(&kmemlock);
    
    return((char *)r);
}

void
check_free_list()
{
    struct run *p;
    if (!kmem.free_list)
        panic("kmem.free_list is a null pointer!\n");

    struct MLOCK * locallock = (struct MLOCK *)malloc(sizeof(struct MLOCK));
    macquire(&kmemlock, locallock);
    // acquire(&kmemlock);

    for (p = kmem.free_list; p; p = p->next) {
        assert((void *)p > (void *)end);
    }

    mrelease(&kmemlock, locallock);
    // release(&kmemlock);
}