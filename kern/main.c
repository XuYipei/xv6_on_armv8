#include <stdint.h>

#include "mmu.h"
#include "arm.h"
#include "string.h"
#include "console.h"
#include "kalloc.h"
#include "vm.h"
#include "trap.h"
#include "timer.h"
#include "spinlock.h"
#include "proc.h"
#include "sd.h"

#include "log.h"
#include "file.h"

struct cpu cpus[NCPU];
uint32_t pgdrinitcnt = 0, pcinitcnt = 0, fsinitcnt = 0;
struct spinlock pgdrinitlock = (struct spinlock){(struct spinlock *)NULL, 0};
struct spinlock pcinitlock = (struct spinlock){(struct spinlock *)NULL, 0};
struct spinlock fsinitlock = (struct spinlock){(struct spinlock *)NULL, 0};

void
main()
{
    /*
     * Before doing anything else, we need to ensure that all
     * static/global variables start out zero.
     */

    extern char edata[], end[], vectors[];

    /*
     * Determine which functions in main can only be
     * called once, and use lock to guarantee this.
     */
    
    cprintf("main: [CPU%d] is init kernel\n", cpuid());

    acquire(&pgdrinitlock);
    if (pgdrinitcnt == 0){
        memset(edata, 0, end - edata);    
        pgdrinitcnt = 1;
    }
    release(&pgdrinitlock);
    
    console_init();
    alloc_init();
    check_free_list();
    irq_init();

    acquire(&pcinitlock);
    if (pcinitcnt == 0){
        proc_init();   
        user_init(0);
        user_init(1);
        user_init(1);
        user_init(1);
        user_init(1);
        pcinitcnt = 1;
    }   
    release(&pcinitlock);

    lvbar(vectors);
    timer_init();

    acquire(&fsinitlock);
    if (fsinitcnt == 0){
        sd_init();
        fsinitcnt = 1;
    }
    release(&fsinitlock);
    // sd_test();        

    cprintf("main: [CPU%d] Init success.\n", cpuid());
    scheduler();
    while (1) ;

}
