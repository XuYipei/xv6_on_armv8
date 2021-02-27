#ifndef INC_SYSCALL_H
#define INC_SYSCALL_H

#include <stdint.h>
#include "syscallno.h"

int argstr(int, char **);
int argint(int, uint64_t *);
int argdir(int, uint64_t *);
int fetchstr(uint64_t, char **);
int fetchint(uint64_t, int64_t *);

int syscall();
int syscall1();

#endif