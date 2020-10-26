#ifndef INC_TRAP_H
#define INC_TRAP_H

#include <stdint.h>

struct trapframe {
    /* TODO: Design your own trapframe layout here. */
    uint64_t *sp;
    uint64_t pc;
};

void trap(struct trapframe *);
void irq_init();
void irq_error();

#endif
