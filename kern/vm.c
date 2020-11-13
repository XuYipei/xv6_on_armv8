#include <stdint.h>
#include "arm.h"
#include "types.h"
#include "mmu.h"
#include "spinlock.h"
#include "string.h"
#include "memlayout.h"
#include "console.h"

#include "vm.h"
#include "kalloc.h"

/* 
 * Given 'pgdir', a pointer to a page directory, pgdir_walk returns
 * a pointer to the page table entry (PTE) for virtual address 'va'.
 * This requires walking the four-level page table structure.
 *
 * The relevant page table page might not exist yet.
 * If this is true, and alloc == false, then pgdir_walk returns NULL.
 * Otherwise, pgdir_walk allocates a new page table page with kalloc.
 *   - If the allocation fails, pgdir_walk returns NULL.
 *   - Otherwise, the new page is cleared, and pgdir_walk returns
 *     a pointer into the new page table page.
 */


uint64_t *
pgdir_walk(uint64_t *pgdir, const void *va, int64_t alloc)
{
    /* TODO: Your code here. */

    uint32_t i;
    uint64_t *tb, *pa = NULL;

    tb = pgdir;
    for (i = 0; i < 3; i++, tb = tb){
        uint32_t idx = PTX(i, va);

        //cprintf("%d %d start\n", cpuid(), i);
        if (tb[idx] == 0 || (tb[idx] & PTE_P) == 0){
            if (!alloc){
                return(NULL);
            }
            //cprintf("???\n");
            uint64_t address = kalloc();
            //cprintf("+++++++++\n");

            tb[idx] = (uint64_t)V2P(address);
            memset((char *)address, 0, PGSIZE);
            tb[idx] |= PTE_P | PTE_TABLE | PTE_AF | PTE_NORMAL;
        }
        //cprintf("%d %d end\n", cpuid(), i);

        uint64_t descriptor = P2V(tb[idx] >> 12 << 12);
        tb = (uint64_t *)descriptor;
    }
    uint64_t *ret = &tb[PTX(3, va)];

    return(ret);
}

/*
 * Create PTEs for virtual addresses starting at va that refer to
 * physical addresses starting at pa. va and size might **NOT**
 * be page-aligned.
 * Use permission bits perm|PTE_P|PTE_TABLE|PTE_AF for the entries.
 *
 * Hint: call pgdir_walk to get the corresponding page table entry
 */

int
map_region(uint64_t *pgdir, void *va, uint64_t size, uint64_t pa, int64_t perm)
{
    /* TODO: Your code here. */

    char *vastart, *vaend, *v;
    vastart = ROUNDDOWN((uint64_t)va, PGSIZE);
    vaend = (uint64_t)va + size;
    uint64_t tail = perm | PTE_P | PTE_TABLE | PTE_AF | PTE_NORMAL;
    for (v = vastart; v < vaend; v += PGSIZE, pa += PGSIZE){
        uint64_t *pte = pgdir_walk(pgdir, v, 1);
        uint64_t pa_ = tail | ((uint64_t)pa >> 12 << 12);
        if (pte == NULL){
            return(-1);
        }            
        if ((uint64_t)*pte & PTE_P){
            return(-1);
        }
        *pte = pa_;
    }

    return(0);
}

/* 
 * Free a page table.
 *
 * Hint: You need to free all existing PTEs for this pgdir.
 */

void
vm_free_dfs(uint64_t *pgdir, int level)
{
    /* TODO: Your code here. */
    uint32_t i;
    for (i = 0; i < PGSIZE; i++){
        if (pgdir[i] != 0 && (pgdir[i] & PTE_P)){
            if (pgdir[i] & PTE_TABLE){
                if (level != 3)
                    vm_free((uint64_t *)(pgdir[i] >> 12 << 12), level + 1);
            }
            pgdir[i] = 0;
        }
    }
    kfree(pgdir);
}

void 
vm_free(uint64_t *pgdir, int level)
{
    /* TODO: Your code here. */

    vm_free_dfs(pgdir, level);

}

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
check_vm()
{
    uint64_t cpuif = cpuid();
    uint64_t *ttbr = kalloc();
    memset((char *)ttbr, 0, PGSIZE);
    modify_ttbr_el1(ttbr, 0);

    uint64_t *ttbr0, *ttbr1;
    ttbr1 = (uint64_t *)load_ttbr_el1(1);
    ttbr0 = (uint64_t *)load_ttbr_el1(0);

    uint64_t va1, va0;
    va1 = 0xFFFF000000006010 + (cpuif << 12);
    *(uint64_t *)va1 = 233;

    map_region(ttbr0, 0x0 + (cpuif << 12), PGSIZE, 0x6000 + (cpuif << 12), 0);
    modify_ttbr_el1(ttbr0, 0);
    va0 = 0x0000000000000010 + (cpuif << 12);

    if (*(uint64_t *)va0 == *(uint64_t *)va1)
        cprintf("CPU %llu: check vm pass.\n", cpuif);
    else
        cprintf("CPU %llu: check vm fail.\n", cpuif);
}