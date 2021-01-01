#include "proc.h"
#include "spinlock.h"
#include "console.h"
#include "kalloc.h"
#include "trap.h"
#include "string.h"
#include "vm.h"
#include "mmu.h"
#include "list.h"

#define HASHENTRIES 17
#define PRIOENTRIES 4

struct {
    struct proc proc[NPROC];
} ptable;

struct list_head hashmp[HASHENTRIES];
struct list_head prioque[PRIOENTRIES];

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
    for (struct list_head *p = hashmp; p < hashmp + HASHENTRIES; p++) {
        list_init(p);
    }
    for (struct list_head *p = prioque; p < hashmp + PRIOENTRIES; p++) {
        list_init(p);
    }
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

    uint64_t sp = p->kstack + PGSIZE;
    
    sp -= sizeof(struct trapframe);
    p->tf = (struct trapframe *)sp;
    memset(p->tf, 0, sizeof(struct trapframe));
    
    sp -= 8;
    *(uint64_t *)sp = (uint64_t)trapret;
    sp -= 8;
    *(uint64_t *)sp = ((uint64_t)p->kstack) + PGSIZE;

    sp -= sizeof(struct context);
    p->context = (struct context *)sp;
    memset(p->context, 0, sizeof(struct context));
    p->context->r30 = (uint64_t)forkret + 8;

    release(&ptablelock);

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
    
    p->clist.next = p->clist.prev = 0;
    p->plist.next = p->plist.prev = 0;

    p->prio  = PRIOENTRIES - 1;
    list_push_back(&prioque[p->prio], &p->plist);
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

int sleepcpu = -1;

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
        
        // cprintf("SCHEDULER TURN BEGIN\n");

        for (struct list_head* l = &prioque[PRIOENTRIES - 1]; l >= prioque; l--) 
            if (!list_empty(l)) {
                // cprintf("FIND PROC\n");

                struct list_head *pl = list_front(l);
                p = container_of(pl, struct proc, plist);

                c->proc = p;
                uvm_switch(p);

                p->state = RUNNING;
                list_delete(&p->plist);

                // cprintf("SWTCH ENTER\n");

                swtch(&c->scheduler, p->context);

                // cprintf("SWTCH RETURN\n");
                break;
            }

        // cprintf("SCHEDULER TURN END\n");

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

void
yield()
{
    acquire(&ptablelock);
    
    struct proc* p = thiscpu->proc;
    p->state = RUNNABLE;
    
    p->intr += 1;
    if (p->intr == (1 << 5) && p->prio == 3)
        p->prio = 2, p->intr = 0;
    if (p->intr == (1 << 10) && p->prio == 2)
        p->prio = 3, p->intr = 0;
    // if (p->intr == (1 << 20) && p->prio == 1)
    //    p->prio = 3, p->intr = 0;

    // list_delete(&p->plist);
    list_push_back(&prioque[p->prio], &p->plist);

    sched();
    release(&ptablelock);
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

    // p->parent->state = RUNNABLE;

    struct proc *pc;
    for (pc = &ptable.proc; pc < &ptable.proc[NPROC]; pc++) {
        if (pc->parent == p) {
            pc->parent = initproc;
        }
    }

    p->state = ZOMBIE;
    list_delete(&p->plist);

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
    
    if (lk != &ptablelock){
        acquire(&ptablelock);
        if (lk) release(lk);
    }

    // cprintf("SLEEP BEGIN\n");

    struct proc *p = thiscpu->proc;
    
    p->chan = chan;
    p->state = SLEEPING;
    list_delete(&p->plist);

    uint64_t entry = (uint64_t)chan % HASHENTRIES;
    list_push_back(&hashmp[entry], &(p->clist));

    // cprintf("SLEEP SCHED\n");
    sched();

    p->chan = 0; 

    if (lk != &ptablelock){
        release(&ptablelock);
        if (lk) acquire(lk);
    }
}

/* Wake up all processes sleeping on chan. */
void
wakeup(void *chan)
{
    /* TODO: Your code here. */
    acquire(&ptablelock);

    uint64_t entry = (uint64_t)chan % HASHENTRIES;
    struct list_head *ery = &hashmp[entry];
    for (struct list_head* l = ery->next; l != ery; l = l->next) {
        struct proc *p = container_of(l, struct proc, clist);
        if (p->chan == chan && p->state == SLEEPING) {
            p->state = RUNNABLE;
            list_delete(l);
            list_push_back(&prioque[p->prio], &p->plist);

            if (p->plist.next == NULL)
                cprintf("WAKEUP NULL\n");
            break;
        }
    }
    
    release(&ptablelock);
}

uint64_t 
currentel()
{
    uint64_t t;
    asm volatile ("mrs %[cnt], CurrentEL" : [cnt]"=r"(t));
    return t;
}

void
proc_disp()
{
    acquire(&ptablelock);
    cprintf("display begin:\n");
    for (struct list_head *l = prioque; l < prioque + PRIOENTRIES; l++) {
        for (struct list_head *pl = l->next; pl != l; pl = pl->next) {
            struct proc *p = container_of(pl, struct proc, plist);
            cprintf("%d %s %d %d\n", p->pid, p->name, p->state, p->prio);
        }
    }
    cprintf("display end.\n");
    release(&ptablelock);
}