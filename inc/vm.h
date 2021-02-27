#ifndef KERN_VM_H
#define KERN_VM_H

#include <stdint.h>
#include "proc.h"

void vm_free(uint64_t *, int);
void uvm_switch(struct proc *);
void uvm_init(uint64_t *, char *, int);
uint64_t *uvm_copy(uint64_t *, uint64_t);

int64_t uvm_alloc(uint64_t *, uint64_t, uint64_t);
int64_t uvm_dealloc(uint64_t *, uint64_t, uint64_t);

uint64_t *pgdir_init();
uint64_t *pgdir_walk(uint64_t *, const void *, int64_t);
uint64_t pgdir_iwalk(uint64_t *, uint64_t);

int map_region(uint64_t *, void *, uint64_t, uint64_t, int64_t);


#endif
