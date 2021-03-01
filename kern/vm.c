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


struct spinlock pgdrlock;

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

uint64_t
pgdir_iwalk(uint64_t *pgdir, uint64_t va)
{
    uint64_t pa;
    uint64_t *pte;
    pte = pgdir_walk(pgdir, va, 0);
    if (pte == 0)
        return(0);
    // pa = V2P(*pte);
    pa = (*pte) >> 12 << 12;
    return(pa);
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
    uint64_t tail = perm | PTE_P | PTE_TABLE | PTE_AF | PTE_NORMAL | PTE_USER | PTE_SH;
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

int64_t
uvm_alloc(uint64_t *pgdir, uint64_t oldsz, uint64_t newsz)
{
    char *pgnew;
    uint64_t a, pa;
    a = ROUNDUP(oldsz, PGSIZE);
    
    for (; a < newsz; a += PGSIZE){
        pgnew = kalloc();
        if (pgnew == 0){
            return(0);
        }
        memset(pgnew, 0, PGSIZE);
        pa = V2P(pgnew);
        if (map_region(pgdir, a, PGSIZE, pa, PTE_USER | PTE_RW) < 0){
            return(0);
        }
    }
    return(newsz);
}

int64_t
uvm_dealloc(uint64_t *pgdir, uint64_t oldsz, uint64_t newsz)
{
    char *pgnew;
    uint64_t a, pa;
    uint64_t *pte;
    a = ROUNDUP(newsz, PGSIZE);
    
    for (; a < oldsz; a += PGSIZE){
        pte = pgdir_walk(pgdir, a, 0);
        if (pte && (*pte) && ((*pte) & (PTE_P))){
            pa = (*pte) >> 12 << 12;
            kfree(P2V(pa));
            *pte = 0;
        }
    }

    return(newsz);
}

void
vm_free_dfs(uint64_t *pgdir, int level)
{
    /* TODO: Your code here. */
    uint32_t i;
    uint64_t descriptor;
    for (i = 0; i < ENTRYSZ; i++){
        if (pgdir[i] != 0 && (pgdir[i] & PTE_P)){
            if (pgdir[i] & PTE_TABLE){
                if (level != 3){
                    descriptor = P2V(pgdir[i] >> 12 << 12);
                    vm_free_dfs((uint64_t *)descriptor, level + 1);
                }
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

uint64_t * 
uvm_copy(uint64_t *old_pgdir, uint64_t sz) {
    uint64_t i, pa, priv;
    uint64_t *new_pgdir, *pte, *new_pg;
    
    if ((new_pgdir = kalloc()) == 0)
        return(0);
    memset(new_pgdir, 0, PGSIZE);

    for (i = 0; i < sz; i += PGSIZE) {
        if ((pte = pgdir_walk(old_pgdir, i, 0)) == 0)
            return(0);
        if (! ((*pte) & PTE_P)) 
            return(0);
        
        pa   = (*pte) >> 12 << 12;
        priv = (*pte) << 52 >> 52;
        
        if ((new_pg = kalloc()) == 0)
            return(0);
        memcpy(new_pg, P2V(pa), PGSIZE);
        
        if (map_region(new_pgdir, i, PGSIZE, V2P(new_pg), priv))
            return(0);
    }
    return(new_pgdir);
};

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