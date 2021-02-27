# Initial process execs /init.

#include "syscallno.h"

.globl start

# while (1)
loop:
    mov     x1, 1
    b       loop

# exec(init, argv)
start:
    mov     x8, #SYS_test
    svc     0x00
    mov     x8, #1
    ldr     x0, =init
    ldr     x1, =argv
    mov     x8, #SYS_execve
    svc     0x00

exit:
    mov     x8, #SYS_exit
    svc     0x00
    b       exit

# char init[] = "/init\0";
init:
    .string "/init\0"

mkfs:
    .string "/mkfs\0"

.p2align 8
funargv:
    .word mkfs
    .word init
    .word 0

# char *argv[] = { init, [mkfs, init], 0 };
.p2align 8
argv:
    .word init
    .word 0
    .word 0
    .word 0
