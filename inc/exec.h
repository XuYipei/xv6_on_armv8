#ifndef KERN_EXEC_H
#define KERN_EXEC_H

#include "syscallno.h"
#include "types.h"
#include "mmu.h"
#include "elf.h"

int execve(char *, char **, char **);

#endif