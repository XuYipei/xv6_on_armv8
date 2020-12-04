#include "proc.h"
#include "spinlock.h"
#include "console.h"
#include "kalloc.h"
#include "trap.h"
#include "string.h"
#include "vm.h"
#include "mmu.h"

struct {
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
void forkret();
extern void trapret();
void swtch(struct context **, struct context *);

/*
 * Initialize the spinlock for ptable to serialize the access to ptable
 */

struct spinlock ptablelock;

void
proc_init()
{
    /* TODO: Your code here. */
    struct proc *p;
    initlock(&ptablelock, "ptable");
}

/*
 * Look through the process table for an UNUSED proc.
 * If found, change state to EMBRYO and initialize
 * state (allocate stack, clear trapframe, set context for switch...)
 * required to run in the kernel. Otherwise return 0.
 */
static struct proc *
proc_alloc()
{
    struct proc *p;
    /* TODO: Your code here. */

    acquire(&ptablelock);

    uint32_t find = 0;
    for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
        if (p -> state == UNUSED){
            find = 1;
            break;
        }
    }
    if (!find) {
        release(&ptablelock);
        return(NULL);
    }

    p->state  = EMBRYO;
    p->pid    = nextpid++;
    p->kstack = kalloc();
    if (p->kstack == NULL) {
        p -> state = UNUSED;
        release(&ptablelock);
        return(NULL);
    }

    uint64_t *sp = p->kstack + PGSIZE;
    
    sp -= sizeof(struct trapframe);
    p->tf = (struct trapframe *)sp;
    memset(p->tf, 0, sizeof(struct trapframe));
    
    sp -= 8;
    *sp = (uint64_t)trapret;

    sp -= sizeof(struct context);
    p->context = (struct context *)sp;
    memset(p->context, 0, sizeof(struct context));
    p->context->r30 = (uint64_t)forkret;

    return p;
}

/*
 * Set up first user process(Only used once).
 * Set trapframe for the new process to run
 * from the beginning of the user process determined 
 * by uvm_init
 */
void
user_init()
{
    struct proc *p;
    /* for why our symbols differ from xv6, please refer https://stackoverflow.com/questions/10486116/what-does-this-gcc-error-relocation-truncated-to-fit-mean */
    extern char _binary_obj_user_initcode_start[], _binary_obj_user_initcode_size[];
    
    /* TODO: Your code here. */
    p = proc_alloc();
    initproc = p;

    if ((p->pgdir = kalloc()) == NULL) {
        panic("userinit");
    }
    
    uvm_init(p->pgdir, _binary_obj_user_initcode_start, _binary_obj_user_initcode_size);
    p->tf->sp  = PGSIZE;

    p->sz = PGSIZE;
    memcpy(p->name, "initcode", sizeof(p->name));
    
    p->state = RUNNABLE;
}

/*
 * Per-CPU process scheduler
 * Each CPU calls scheduler() after setting itself up.
 * Scheduler never returns.  It loops, doing:
 *  - choose a process to run
 *  - swtch to start running that process
 *  - eventually that process transfers control
 *        via swtch back to the scheduler.
 */
void
scheduler()
{
    struct proc *p;
    struct cpu *c = thiscpu;
    c->proc = NULL;
    
    for (;;) {
        /* Loop over process table looking for process to run. */
        /* TODO: Your code here. */
        acquire(&ptablelock);
        
        for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
            if (p->state != RUNNABLE)
                continue;

            c->proc = p;
            uvm_switch(p->pgdir);

            p->state = RUNNING;

            cprintf("%llx %llx\n", p->context->r30, (uint64_t)forkret);
            swtch(&c->scheduler, p->context);

        }

        release(&ptablelock);
    }
}

/*
 * Enter scheduler.  Must hold only ptable.lock
 */
void
sched()
{
    /* TODO: Your code here. */
    
    if (!holding(&ptablelock)) {
        panic("sched");
    }

    struct cpu *c = thiscpu;

    swtch(&c->proc->context, c->scheduler);
    
}

/*
 * A fork child will first swtch here, and then "return" to user space.
 */
void
forkret()
{
    /* TODO: Your code here. */
    release(&ptablelock);
}

void
wakeup(struct proc *p)
{
    p->state = RUNNABLE;
}

/*
 * Exit the current process.  Does not return.
 * An exited process remains in the zombie state
 * until its parent calls wait() to find out it exited.
 */
void
exit()
{
    struct proc *p = thiscpu->proc;
    /* TODO: Your code here. */
        acquire(&ptablelock);

    wakeup(p->parent);

    struct proc *pc;
    for (pc = &ptable.proc; pc < &ptable.proc[NPROC]; pc++) {
        if (pc->parent == p) {
            pc->parent = initproc;
        }
    }

    p->state = ZOMBIE;
    sched();

}

/*
 * Atomically release lock and sleep on chan.
 * Reacquires lock when awakened.
 */
void
sleep(void *chan, struct spinlock *lk)
{
    /* TODO: Your code here. */
}

/* Wake up all processes sleeping on chan. */
void
wakeup(void *chan)
{
    /* TODO: Your code here. */
}

