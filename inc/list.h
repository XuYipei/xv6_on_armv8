#ifndef INC_LIST_H
#define INC_LIST_H

#include "stddef.h"

#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#define container_of(p,t,m) ((t*)((char *)(p)-offsetof(t,m)))
#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct list_head{
    struct list_head* next, *prev;
};

void 
list_push_front(struct list_head *, struct list_head *);

void 
list_push_back(struct list_head *, struct list_head *);

struct list_head *
list_pop_front(struct list_head *);

struct list_head *
list_pop_back(struct list_head *);

struct list_head *
list_front(struct list_head *);

struct list_head *
list_back(struct list_head *);

int 
list_empty(struct list_head *);

void 
list_init(struct list_head *);

#endif