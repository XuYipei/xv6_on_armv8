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
uint32_t pgdrinitcnt = 0, pcinitcnt = 0;
struct spinlock pgdrinitlock = (struct spinlock){(struct spinlock *)NULL, 0};
struct spinlock pcinitlock = (struct spinlock){(struct spinlock *)NULL, 0};

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
    
    if (cpuid() == 0){
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
            user_init();
            pcinitcnt = 1;
        }
        release(&pcinitlock);

        lvbar(vectors);
        timer_init();

        sd_init();
        sd_test();        

        cprintf("main: [CPU%d] Init success.\n", cpuid());
        scheduler();
        while (1) ;
    }
}
