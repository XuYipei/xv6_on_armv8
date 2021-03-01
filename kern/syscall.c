#include "syscall.h"
#include "string.h"
#include "proc.h"
#include "console.h"
#include "sd.h"

#include "types.h"
#include "vm.h"
#include "mmu.h"

/* 
 * User code makes a system call with SVC, system call number in r0.
 * Arguments on the stack, from the user call to the C
 * library system call function.
 */

/* Fetch the int at addr from the current process. */


int
fetchint(uint64_t addr, int64_t *ip)
{
    struct proc *proc = thiscpu->proc;

    if (addr >= proc->sz || addr + 8 > proc->sz) {
        return -1;
    }
    *ip = *(int64_t *)(addr);
    return 0;
}

int
copyinstr(char *dst, uint64_t *src)
{
    char *s, *t;
    uint32_t n;

    s = (char *)src;
    t = dst;
    for (n = 0; ;){
        if (*s == '\0'){
            *t = '\0';
            return (n);
        }else{
            *t = *s;
        }
        t++;
        s++;
        n++;
    }
    return(-1);
}

/* 
 * Fetch the nul-terminated string at addr from the current process.
 * Doesn't actually copy the string - just sets *pp to point at it.
 * Returns length of string, not including nul.
 */
int
fetchstr(uint64_t addr, char **pp)
{
    struct proc *proc;
    char *s, *ep;
    
    proc = thiscpu->proc;
    if (addr >= proc->sz) {
        return -1;
    }

    *pp = (char *)addr;
    ep = (char *)proc->sz;

    for (s = *pp; s < ep; s++) {
        if (*s == 0) {
            return s - *pp;
        }
    }

    return -1;
}

/* 
 * Fetch the nth (starting from 0) 32-bit system call argument.
 * In our ABI, r0 contains system call index, r1-r4 contain parameters.
 * now we support system calls with at most 4 parameters.
 */
int
argint(int n, uint64_t *ip)
{
    if (n > 3) {
        panic("argint: too many system call parameters\n");
    }

    struct proc *proc = thiscpu->proc;

    *ip = *(&proc->tf->r0 + n);

    return 0;
}

/* 
 * Fetch the nth word-sized system call argument as a pointer
 * to a block of memory of size n bytes.  Check that the pointer
 * lies within the process address space.
 */
int
argptr(int n, char **pp, int size)
{
    uint64_t i;

    if (argint(n, &i) < 0) {
        return -1;
    }

    struct proc *proc = thiscpu->proc;

    if ((uint64_t)i >= proc->sz || (uint64_t)i + size > proc->sz) {
        return -1;
    }

    *pp = (char*)i;
    return 0;
}

/* 
 * Fetch the nth word-sized system call argument as a string pointer.
 * Check that the pointer is valid and the string is nul-terminated.
 * (There is no shared writable memory, so the string can't change
 * between this check and being used by the kernel.)
 */
int
argstr(int n, char **pp)
{
    uint64_t addr;
    if (argint(n, &addr) < 0) {
        return -1;
    }
    return fetchstr(addr, pp);
}

int
sys_rt_sigprocmask()
{
    return(0);
}

extern int sys_exec();
extern int sys_exit();

/* 
 * in ARM, parameters to main (argc, argv) are passed in r0 and r1
 * do not set the return value if it is SYS_exec (the user program
 * anyway does not expect us to return anything).
 */
/* syscall handler */
int
syscall()
{
    struct proc *proc = thiscpu->proc;
    int sysno = proc->tf->r8;

    // cprintf("syscall: %d\n", sysno);

    int ret = 0;
    switch (sysno) {
        // FIXME: Use pid instead of tid since we don't have threads :)
        case SYS_set_tid_address:
        case SYS_gettid:
            ret = thisproc()->pid;
            break;

        // FIXME: Hack TIOCGWINSZ(get window size)
        case SYS_ioctl:
            if (proc->tf->r1 == 0x5413){
                ret = 0;
                break;
            }
            else panic("ioctl unimplemented. 1");

        // FIXME: Always return 0 since we don't have signals  :)
        case SYS_rt_sigprocmask:
            ret = sys_rt_sigprocmask();
            break;
        case SYS_brk:
            ret = sys_brk();
            break;
        case SYS_execve:
            ret = sys_exec();
            break;
        case SYS_sched_yield:
            ret = sys_yield();
            break;
        case SYS_clone:
            ret = sys_clone();
            break;
        case SYS_wait4:
            ret = sys_wait4();
            break;
        // FIXME: exit_group should kill every thread in the current thread group.
        case SYS_exit_group:
        case SYS_exit:
            ret = sys_exit();
            break;
        case SYS_dup:
            ret = sys_dup();
            break;
        case SYS_chdir:
            ret = sys_chdir();
            break;
        case SYS_fstat:
            ret = sys_fstat();
            break;
        case SYS_newfstatat:
            ret = sys_fstatat();
            break;
        case SYS_mkdirat:
            ret = sys_mkdirat();
            break;
        case SYS_mknodat:
            ret = sys_mknodat();
            break;
        case SYS_openat:
            ret = sys_openat();
            break;
        case SYS_writev:
            ret = sys_writev();
            break;
        case SYS_write:
            ret = sys_write();
            break;
        case SYS_read:
            ret = sys_read();
            break;
        case SYS_close:
            ret = sys_close();
            break;
        case SYS_test:
            ret = sys_test();
            break;
        default:
            // debug_reg();
            panic("Unexpected syscall #%d\n", sysno);
            ret = 0;
            break;
    }

    proc->tf->r0 = ret;
    return(ret);
}

/* TODO: If you want to use musl
 *
 * 1. Remove inc/syscall.h and inc/syscallno.h
 * 2. Include <syscall.h> from musl.
 * 3. Rename `syscall()` to `syscall1()`.
 * 4. Hack some syscalls for musl as follows.
 *
 * ```
 * int
 * syscall1(struct trapframe *tf)
 * {
 *     thisproc()->tf = tf;
 *     int sysno = tf->x[8];
 *     switch (sysno) {
 * 
 *     // FIXME: Use pid instead of tid since we don't have threads :)
 *     case SYS_set_tid_address:
 *     case SYS_gettid:
 *         return thisproc()->pid;
 * 
 *     // FIXME: Hack TIOCGWINSZ(get window size)
 *     case SYS_ioctl:
 *         if (tf->x[1] == 0x5413) return 0;
 *         else panic("ioctl unimplemented. ");
 * 
 *     // FIXME: Always return 0 since we don't have signals  :)
 *     case SYS_rt_sigprocmask:
 *         return 0;
 * 
 *     case SYS_brk:
 *         return sys_brk();
 * 
 *     case SYS_execve:
 *         return sys_exec();
 * 
 *     case SYS_sched_yield:
 *         return sys_yield();
 * 
 *     case SYS_clone:
 *         return sys_clone();
 * 
 *     case SYS_wait4:
 *         return sys_wait4();
 * 
 *     // FIXME: exit_group should kill every thread in the current thread group.
 *     case SYS_exit_group:
 *     case SYS_exit:
 *         return sys_exit();
 * 
 *     case SYS_dup:
 *         return sys_dup();
 * 
 *     case SYS_chdir:
 *         return sys_chdir();
 * 
 *     case SYS_fstat:
 *         return sys_fstat();
 * 
 *     case SYS_newfstatat:
 *         return sys_fstatat();
 * 
 *     case SYS_mkdirat:
 *         return sys_mkdirat();
 * 
 *     case SYS_mknodat:
 *         return sys_mknodat();
 *         
 *     case SYS_openat:
 *         return sys_openat();
 * 
 *     case SYS_writev:
 *         return sys_writev();
 * 
 *     case SYS_read:
 *         return sys_read();
 * 
 *     case SYS_close:
 *         return sys_close();
 * 
 *     default:
 *         // FIXME: don't panic.
 * 
 *         debug_reg();
 *         panic("Unexpected syscall #%d\n", sysno);
 *         
 *         return 0;
 *     }
 * }
 * ```
 *
 */

