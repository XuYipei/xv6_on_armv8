#include <stdint.h>

#include "proc.h"
#include "trap.h"
#include "console.h"

#include "vm.h"
#include "log.h"
#include "file.h"

int
sys_yield()
{
    yield();
    return 0;
}

size_t
sys_brk()
{
    /* TODO: Your code here. */
    struct proc *proc;
    int64_t n, oldsz, cursz;
    if (argint(0, &n) < 0)
        return -1;

    proc  = thiscpu->proc;
    oldsz = proc->sz;

    if (n >= 0){
        if (cursz = uvm_alloc(proc->pgdir, oldsz, oldsz + n) == 0) 
            return(-1);
    }else{
        if (cursz = uvm_alloc(proc->pgdir, oldsz, oldsz + n) == 0)
            return(-1);
    }

    proc->sz = cursz;
    lttbr0(proc->pgdir);

    return oldsz;
}

int
sys_clone()
{
    void *childstk;
    uint64_t flag;
    if (argint(0, &flag) < 0 || argint(1, &childstk) < 0)
        return -1;
    if (flag != 17) {
        cprintf("sys_clone: flags other than SIGCHLD are not supported.\n");
        return -1;
    }
    return fork();
}


int
sys_wait4()
{
    int64_t pid, opt;
    int *wstatus;
    void *rusage;
    if (argint(0, &pid) < 0 ||
        argint(1, &wstatus) < 0 ||
        argint(2, &opt) < 0 ||
        argint(3, &rusage) < 0)
        return -1;

    if (pid != -1 || wstatus != 0 || opt != 0 || rusage != 0) {
        cprintf("sys_wait4: unimplemented. pid %d, wstatus 0x%p, opt 0x%x, rusage 0x%p\n", pid, wstatus, opt, rusage);
        return -1;
    }

    return wait();
}

int
sys_exit()
{
    cprintf("sys_exit: in exit\n");
    exit();
    return 0;
}

int fsmnt = 0;

int
sys_test()
{  
    binit();
    iinit(1);
    fileinit();

    struct proc *proc = thiscpu->proc;
    proc->cwd = namei("/");

    return(0);
}