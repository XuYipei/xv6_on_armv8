#include <stdint.h>

#include "arm.h"
#include "string.h"
#include "console.h"
#include "kalloc.h"
<<<<<<< HEAD
#include "trap.h"
#include "timer.h"
=======
#include "vm.h"
#include "mmu.h"

extern char end[];

static inline uint64_t
load_ttbr_el1(uint32_t sp)
{
    // asm volatile("mrs vbar_el1, %[1]": [x]"=r"(p) :);
    uint64_t ttbr = 0;
    if (sp == 0)
        asm volatile("mrs %[dest], ttbr0_el1": [dest]"=r"(ttbr) : );
    if (sp == 1)
        asm volatile("mrs %[dest], ttbr1_el1": [dest]"=r"(ttbr) : );
    return(ttbr);
}

static inline void
modify_ttbr_el1(uint64_t *ttbr, uint32_t sp)
{  
    if (sp == 0)
        asm volatile("msr ttbr0_el1, %[src]": : [src]"r"(ttbr));
    if (sp == 1)
        asm volatile("msr ttbr1_el1, %[src]": : [src]"r"(ttbr));
    
    asm volatile("ic iallu": : );
    asm volatile("tlbi vmalle1is": : );
    asm volatile("dsb ish": : );
}
>>>>>>> 1c76857... lab 2

void
main()
{
    /*
     * Before doing anything else, we need to ensure that all
     * static/global variables start out zero.
     */

    extern char edata[], end[], vectors[];

    /* TODO: Use `memset` to clear the BSS section of our program. */
    memset(edata, 0, end - edata);    
    /* TODO: Use `cprintf` to print "hello, world\n" */

    /* Init free pages. */
    console_init();
    alloc_init();
    cprintf("Allocator: Init success.\n");
    check_free_list();

    irq_init();

    lvbar(vectors);
    timer_init();

    while (1) ;
}
