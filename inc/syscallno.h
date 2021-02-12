#ifndef INC_SYSCALLNO_H
#define INC_SYSCALLNO_H

#define MAXARG       32  /* max exec arguments */

#define SYS_exec        0
#define SYS_exit        1
#define SYS_brk         2
#define SYS_wait4       3
#define SYS_sched_yield 4
#define SYS_dup         5
#define SYS_fstat       6
#define SYS_mkdirat     7
#define SYS_mknodat     8
#define SYS_openat      9
#define SYS_writev      10
#define SYS_read        11
#define SYS_close       12      
#define SYS_test        13                                       

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

#endif