#include <stdint.h>

#include "string.h"
#include "console.h"
#include "kalloc.h"
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

void
main()
{
    /*
     * Before doing anything else, we need to ensure that all
     * static/global variables start out zero.
     */

    extern char edata[], end[];

    /* TODO: Use `memset` to clear the BSS section of our program. */
    memset(edata, 0, end - edata);    
    /* TODO: Use `cprintf` to print "hello, world\n" */

    /* Init free pages. */
    console_init();
    alloc_init();
    cprintf("Allocator: Init success.\n");

    uint64_t *a = balloc(4 << 12);
    uint64_t *b = balloc(4 << 12);
    bfree(a, 4 << 12);
    bfree(b, 2 << 12);
    bfree((uint64_t)b + (2 << 12), 2 << 12);
    uint64_t *c = balloc(8 << 12);
    cprintf("%llx %llx\n", a, c);

    /*
     * Load ttbr1_el1 and write 233 into va=0xFFFF000000001000, which
     * means write 233 into pa=0x0000000000001000.
    */
    uint64_t *ttbr1_el1;
    ttbr1_el1 = (uint64_t *)load_ttbr_el1(1);
    //cprintf("%llx\n", (uint64_t)ttbr1_el1);
    uint64_t *va;
    va = 0xFFFF000000001010;
    *va = 233;
    cprintf("%llu\n", *va);

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

    while (1) ;
}
