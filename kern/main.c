#include <stdint.h>

#include "arm.h"
#include "string.h"
#include "console.h"
#include "kalloc.h"
#include "vm.h"
#include "trap.h"
#include "timer.h"
#include "spinlock.h"

uint32_t pgdrinitcnt;
struct spinlock pgdrlock;

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

    /* TODO: Use `memset` to clear the BSS section of our program. */
    memset(edata, 0, end - edata);    
    /* TODO: Use `cprintf` to print "hello, world\n" */
    console_init();
    alloc_init();
    cprintf("Allocator: Init success.\n");
    check_free_list();

    irq_init();

    lvbar(vectors);
    timer_init();

    cprintf("CPU %d: Init success.\n", cpuid());


    acquire(&pgdrlock);

    if (pgdrinitcnt == 0){
        
        pgdrinitcnt = 1;
    }

    release(&pgdrlock);

    while (1) ;
}
