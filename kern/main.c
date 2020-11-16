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

struct cpu cpus[NCPU];
uint32_t pgdrinitcnt = 0, pcinitcnt = 0, sdinitcnt = 0;
struct spinlock pgdrinitlock = (struct spinlock){(struct spinlock *)NULL, 0};
struct spinlock pcinitlock = (struct spinlock){(struct spinlock *)NULL, 0};
struct spinlock sdinitlock = (struct spinlock){(struct spinlock *)NULL, 0};

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
    /* TODO: Your code here. */
    
    cprintf("main: [CPU%d] is init kernel\n", cpuid());

    /* TODO: Use `memset` to clear the BSS section of our program. */

    acquire(&pgdrinitlock);
    if (pgdrinitcnt == 0){
        memset(edata, 0, end - edata);    
        pgdrinitcnt = 1;
        cprintf("init bss in CPU %d.\n", cpuid());
    }
    release(&pgdrinitlock);
    
    /* TODO: Use `cprintf` to print "hello, world\n" */
    console_init();
    alloc_init();
    check_free_list();

    irq_init();
    acquire(&pcinitlock);
    if (pcinitcnt == 0){
        proc_init();
        pcinitcnt = 1;
        
        user_init();
        user_init();
        user_init();
        user_init();
        // user_init();
        // user_init();
        // user_init();
    }   
    release(&pcinitlock);

<<<<<<< HEAD
    lvbar(vectors);
    timer_init();
=======
    /*
     * Modify ttbr0_el1 into a new user space pointer ttbr.
     * In ttbr, va=0x0000000000000000 points to pa=0x0000000000001000.
     * Therefore, if we load the data in va=0x0000000000000000, we can get 233.
    */
    uint64_t *ttbr = kalloc();
    memset((char *)ttbr, 0, PGSIZE);
    map_region(ttbr, 0, PGSIZE, 0x1000, 0);
    modify_ttbr_el1(ttbr, 0);
    va = 0x0000000000000010;
    cprintf("%llu\n", *va);
>>>>>>> 106b992... rebase dev2

    acquire(&sdinitlock);
    if (sdinitcnt == 0){
        sd_init();
        sdinitcnt = 1;
    }
    release(&sdinitlock);
    // sd_test();        

    cprintf("main: [CPU%d] Init success.\n", cpuid());
    scheduler();
    while (1) ;

}
