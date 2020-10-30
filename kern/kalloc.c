#include <stdint.h>
#include "types.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "console.h"
#include "kalloc.h"

extern char end[];

struct run {
    struct run *next;
};

struct {
    struct run *free_list;
} kmem;

void
alloc_init()
{
    free_range(end, P2V(PHYSTOP));
}

void
kfree(char *v)
{
    struct run *r;

    if ((uint64_t)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
        panic("kfree");

    // memset(v, 1, PGSIZE);
    
    r = (struct run *)v;
    r->next = kmem.free_list;
    kmem.free_list = r;
}

void
free_range(void *vstart, void *vend)
{
    //cprintf("%llx\n", (uint64_t)vstart);
    //cprintf("%llx\n", (uint64_t)vend);

    char *p;
    p = ROUNDUP((char *)vstart, PGSIZE);
    for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
        kfree(p);
}

char *
kalloc()
{
    if (kmem.free_list == NULL) 
        return(0);

    struct run *r = kmem.free_list; 
    kmem.free_list = r->next;
    return((char *)r);
}

void
check_free_list()
{
    struct run *p;
    if (!kmem.free_list)
        panic("'kmem.free_list' is a null pointer!");

    for (p = kmem.free_list; p; p = p->next) {
        assert((void *)p > (void *)end);
    }
}