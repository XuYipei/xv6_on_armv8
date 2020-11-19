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
#include "proc.h"

extern uint64_t *kpgdir;

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
            uint64_t address = kalloc();

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
 * Use permission bits perm|PTE_P|PTE_TABLE|(MT_NORMAL << 2)|PTE_AF|PTE_SH for the entries.
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

/* Get a new page table */
uint64_t *
pgdir_init()
{
    /* TODO: Your code here. */

    uint64_t *pgdir =  kalloc();

    if (pgdir == NULL) {
        panic("pgdirinit");
    }

    return(pgdir);
}

/* 
 * Load binary code into address 0 of pgdir.
 * sz must be less than a page.
 * The page table entry should be set with
 * additional PTE_USER|PTE_RW|PTE_PAGE permission
 */
void
uvm_init(uint64_t *pgdir, char *binary, int sz)
{
    /* TODO: Your code here. */

    char *r = kalloc();
    memset(r, 0, PGSIZE);
    map_region(pgdir, 0, PGSIZE, V2P(r), PTE_RW);

    memmove(r, binary, sz);
}

/*
 * switch to the process's own page table for execution of it
 */
void
uvm_switch(struct proc *p)
{
    /* TODO: Your code here. */
    uint64_t r = V2P((uint64_t)p->pgdir);
    lttbr0(r);
}