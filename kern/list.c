#include "list.h"

void 
list_push_front(struct list_head *que, struct list_head *ptr) {
    if (que->next == que){
        que->prev = que->next = ptr;
        ptr->prev = ptr->next = que;
        return;
    }
    ptr->next = que->next;
    ptr->prev = que; 
    que->next->prev = ptr;
    que->next = ptr;
}

void 
list_push_back(struct list_head *que, struct list_head *ptr) {
    if (que->next == que){
        que->prev = que->next = ptr;
        ptr->prev = ptr->next = que;
        return;
    }
    ptr->prev = que->prev;
    ptr->next = que; 
    que->prev->next = ptr;
    que->prev = ptr;
}

struct list_head *
list_pop_front(struct list_head *que) {
    struct list_head *ptr;
    ptr = que->next;
    que->next = que->next->next;
    que->next->prev = que;
    return(ptr);
}

struct list_head *
list_pop_back(struct list_head *que) {
    struct list_head *ptr;
    ptr = que->prev;
    que->prev = que->prev->prev;
    que->prev->next = que;
    return(ptr);
}

struct list_head *
list_front(struct list_head *que) {
    return (que->next);
}

struct list_head *
list_back(struct list_head *que) {
    return (que->prev);
}

int 
list_empty(struct list_head *que){
    return(que->next == que);
}

void 
list_init(struct list_head *que){
    que->next = que->prev = que;
}