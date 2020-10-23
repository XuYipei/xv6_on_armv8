#include <stdint.h>
#include "types.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "console.h"
#include "kalloc.h"

extern char end[];

/* 
 * Free page's list element struct.
 * We store each free page's run structure in the free page itself.
 */
struct buddy_t {
    struct buddy_t *next, *prev;
};

struct buddy_allocator_t {
    struct buddy_t link_head[64];
    uint64_t maxsize;
    uint32_t maxlevel;
    struct buddy_t *astart, *aend;
    uint8_t *status;
};

struct buddy_allocator_t *allocator;

void 
buddy_link_del(struct buddy_t *a)
{
    if (a->next)
        a->next->prev = a->prev;
    if (a->prev)
        a->prev->next = a->next;
    a->next = a->prev = NULL;
}

void
buddy_link_ins(struct buddy_t *head, struct buddy_t *b)
{
    head->next->prev = b;
    b->next = head->next;
    head->next = b;
    b->prev = head;
}

uint32_t 
buddy_level(uint64_t size)
{
    if (size == 0)
        return(0);
    size >>= 12;
    uint32_t level = 0;
    for (; size; size >>= 1)
        level += 1;
    return(level - 1);
}

struct buddy_t *
buddy_left(struct buddy_t *x, struct buddy_t *y, uint32_t level)
{
    if ((uint64_t)x >> level & 1)
        return(x);
    return(y);
}

uint64_t
buddy_index(struct buddy_allocator_t *allocator, struct buddy_t *x)
{
    return(((uint64_t)x - (uint64_t)allocator->astart) >> 12); 
}

uint64_t
buddy_size(uint64_t s)
{
    uint64_t log = buddy_level(s);
    if ((1LLU << log << 12) < s)
        return(1LLU << (log + 1) << 12);
    else 
        return(1LLU << log << 12);
}

struct buddy_allocator_t *
buddy_allocator_init(void *vstart, void *vend)
{
    struct buddy_allocator_t *r = vstart;
    r->astart    = ROUNDUP((uint64_t)&(r->status) + sizeof(uint8_t) * (((uint64_t)vend - (uint64_t)vstart + 1) / PGSIZE), PGSIZE);
    r->aend      = ROUNDUP(vend, PGSIZE) - PGSIZE;
    r->maxsize   = (uint64_t)r->aend - (uint64_t)r->astart + 1;
    r->maxlevel  = buddy_level(r->maxsize);

    uint64_t i, j, k, size;
    for (i = 0; i < 64; i += 1){
        r->link_head[i].next = &(r->link_head[i]);
        r->link_head[i].prev = &(r->link_head[i]);
    }

    uint8_t * status_p;
    status_p = r->status;
    size = ((uint64_t)r->aend - (uint64_t)r->astart + 1) / PGSIZE;
    for (i = 0; i < size; i += 1, status_p += 1){
        r->status[i] = 0;
    }
    for (i = (uint64_t)r->astart, j = 63; ;j -= 1){
        if (size >> j & 1){
            k = (i - (uint64_t)r->astart) / PGSIZE;
            *(r->status + k) = j + 1;
            buddy_link_ins(&(r->link_head[j]), (struct buddy_t *)i);
            i += (1 << j + 12);
        }
        if (j == 0)
            break;
    }
    
    return(r);
}

uint8_t
buddy_valid(struct buddy_allocator_t *allocator, uint64_t v)
{
    if (v % PGSIZE || v < (uint64_t)allocator->astart || v > (uint64_t)allocator->aend)
        return(0);
    return(1);
}

uint64_t
buddy_address(struct buddy_allocator_t *allocator, void *v, uint8_t level)
{
    uint64_t d = (uint64_t)v - (uint64_t)allocator->astart;
    return((uint64_t)allocator->astart + (d ^ (1LLU << level << 12)));
    
}

void
buddy_free(struct buddy_allocator_t *allocator, char *v, uint64_t size)
{
    struct buddy_t *r;

    if (!buddy_valid(allocator, (uint64_t)v))
        panic("kfree");
    if (!buddy_valid(allocator, (uint64_t)v + size))
        panic("kfree");
    
    size = buddy_size(size);
    uint32_t level = buddy_level(size);
    for (; level < 64; level += 1){
        struct buddy_t *buddy; 
        buddy = (struct buddy_t *)buddy_address(allocator, v, level);

        if (!buddy_valid(allocator, (uint64_t)v + (1LLU << level << 12)))
            break;
        if (!buddy_valid(allocator, (uint64_t)buddy + (1LLU << level << 12)))
            break;
        if (allocator->status[buddy_index(allocator, buddy)] != level + 1){
            allocator->status[buddy_index(allocator, v)] = level + 1;
            break;
        }

        buddy_link_del(buddy);
        buddy_link_del(v);
        allocator->status[buddy_index(allocator, buddy)] = 0;
        allocator->status[buddy_index(allocator, v)] = 0;
        
        v = buddy_left(buddy, v, level);
        allocator->status[buddy_index(allocator, v)] = level + 2;
        buddy_link_ins(&(allocator->link_head[level + 1]), v);
    }
}

struct buddy_t *
buddy_alloc(struct buddy_allocator_t *allocator, uint64_t size)
{
    struct buddy_t *r;
    size = buddy_size(size);

    uint32_t level = buddy_level(size), level_org = level;
    for (; level < 64; level += 1)
        if (allocator->link_head[level].next != &(allocator->link_head[level]))
            break;
    if (level == 64)
        return(NULL);

    r = allocator->link_head[level].next;
    buddy_link_del(r);
    allocator->status[buddy_index(allocator, r)] = 0;
    for (; level > level_org; level -= 1){
        struct buddy_t *right = (uint64_t)r + (1LLU << level - 1 + 12);
        buddy_link_ins(&(allocator->link_head[level - 1]), right);
        allocator->status[buddy_index(allocator, right)] = level;
    }
    
    return(r);
}

void
free_range(void *vstart, void *vend)
{
    //cprintf("%llx\n", (uint64_t)vstart);
    //cprintf("%llx\n", (uint64_t)vend);
    char *p;
    vstart = ROUNDUP((char *)vstart, PGSIZE);
    vend   = ROUNDUP((char *)vend, PGSIZE);
    
    uint64_t size = (uint64_t)vend - (uint64_t)vstart + 1;
    size = buddy_size(size);

    buddy_free(allocator, vstart, size);
}

void 
kfree(char *v)
{
    free_range(v, (uint64_t)v + (1 << 12) - 1);
}

char *
kalloc()
{
    /* TODO: Your code here. */
    return(buddy_alloc(allocator, 1 << 12));
}

void 
alloc_init()
{
    allocator = buddy_allocator_init(end ,P2V(PHYSTOP));
}

char *
balloc(uint64_t size)
{
    return(buddy_alloc(allocator, size));
}

void
bfree(void *v, uint64_t size)
{
    buddy_free(allocator, v, size);
}

/*
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

    memset(v, 1, PGSIZE);
    
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

*/