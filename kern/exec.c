#include <elf.h>

#include "trap.h"

#include "file.h"
#include "log.h"
#include "string.h"

#include "console.h"
#include "vm.h"
#include "proc.h"
#include "memlayout.h"

#include "arm.h"
#include "kalloc.h"
#include "exec.h"

static int
loadseg(uint64_t* pgdir, uint64_t va, struct inode *ip, uint32_t off, uint32_t sz)
{
    cprintf("Loadseg begin:\n");

    uint32_t i, s;
    uint64_t pa, a;
    for (i = 0; i < sz; i += PGSIZE){
        s = (sz - i >= PGSIZE) ? (PGSIZE) : (sz - i);
        if (readi(ip, va + i, off + i, s) != s)
            return(-1);
        cprintf("%x\n", i);
    }
    return(0);
}

int
copyout(uint64_t *pgdir, uint64_t dstva, char *src, int64_t len)
{
    uint64_t n, va0;

    while (len > 0) {
        va0 = ROUNDDOWN(dstva, PGSIZE);
        n = PGSIZE - (dstva - va0);
        if (n > len) n = len;
        memcpy(dstva, src, n);

        len -= n;
        src += n;
        dstva = va0 + PGSIZE;
    }
    return(0);
}

int
execve(char *path, char * argv[], char * envp[])
{
    /* TODO: Load program into memory. */

    Elf64_Ehdr eh;
    Elf64_Phdr ph;
    struct inode *ip;
    int32_t sz, i, off, argc, ret;
    struct proc *proc;
    uint64_t sp, stackbase, ustack[MAXARG];
    uint64_t *pgdir, *pgdir_old;
    
    begin_op();
    if ((ip = namei(path)) == 0){
        end_op();
        return(-1);
    }
    ilock(ip);

    if (readi(ip, &eh, 0, sizeof(eh)) != sizeof(eh))
        return(-1);
    
    sz = 0;
    proc = thisproc();
    pgdir = kalloc();
    pgdir_old = proc->pgdir;
    for (i = 0, off = eh.e_phoff; i < eh.e_phnum; i++, off += sizeof(ph)){
        if (readi(ip, &ph, off, sizeof(ph)) != sizeof(ph))
            return(-1);
        if (ph.p_type != PT_LOAD)
            continue;
        if ((sz = uvm_alloc(pgdir, sz, ph.p_memsz + ph.p_vaddr)) == 0)
            return(-1);

        proc->pgdir = pgdir;
        lttbr0((uint64_t)pgdir);

        if (loadseg(pgdir, ph.p_vaddr, ip, ph.p_offset, ph.p_filesz) != 0)
            return(-1);
    }

    iunlock(ip);
    end_op();

    /* TODO: Allocate user stack. */
    uint64_t oldsz = proc->sz;
    sz = ROUNDUP(sz, PGSIZE);
    if ((sz = uvm_alloc(pgdir, sz, sz + 2 * PGSIZE)) == 0)
        return(-1);
    lttbr0((uint64_t)pgdir);
    sp = sz;
    stackbase = sp - PGSIZE;

    /* TODO: Push argument strings.*/
    
    for (argc = 0; argv[argc]; argc++) {
        sp -= (strlen(argv[argc]) + 1); // '0 
        sp -= sp % 16;
        copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1);
        ustack[argc + 1] = sp;
    }
    ustack[0] = argc;
    ustack[argc + 1] = 0;
    ustack[argc + 2] = 0;
    ustack[argc + 3] = AT_PAGESZ;
    ustack[argc + 4] = PGSIZE;
    ustack[argc + 5] = AT_NULL;
    ustack[argc + 6] = 0;

    sp -= (argc + 7) * sizeof(uint64_t);
    sp -= sp % 16;
    if (copyout(pgdir, sp, ustack, (argc + 7) * sizeof(uint64_t)) < 0)
        return(-1);

    proc->tf->r0 = argc;
    proc->tf->r1 = sp;

    memcpy(proc->name, path, sizeof(proc->name));

    vm_free(pgdir_old, 0);
    
    proc->sz = sz;
    proc->tf->pc = eh.e_entry;
    proc->tf->sp = sp;
    
    return(argc);

    /* TODO: Push argument strings.
     *
     * The initial stack is like
     *
     *   +-------------+
     *   | auxv[o] = 0 | 
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   auxv[0]   |
     *   +-------------+
     *   | envp[m] = 0 |
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   envp[0]   |
     *   +-------------+
     *   | argv[n] = 0 |  n == argc
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   argv[0]   |
     *   +-------------+
     *   |    argc     |
     *   +-------------+  <== sp
     *
     * where argv[i], envp[j] are 8-byte pointers and auxv[k] are
     * called auxiliary vectors, which are used to transfer certain
     * kernel level information to the user processes.
     *
     * ## Example 
     *
     * ```
     * sp -= 8; *(size_t *)sp = AT_NULL;
     * sp -= 8; *(size_t *)sp = PGSIZE;
     * sp -= 8; *(size_t *)sp = AT_PAGESZ;
     *
     * sp -= 8; *(size_t *)sp = 0;
     *
     * // envp here. Ignore it if your don't want to implement envp.
     *
     * sp -= 8; *(size_t *)sp = 0;
     *
     * // argv here.
     *
     * sp -= 8; *(size_t *)sp = argc;
     *
     * // Stack pointer must be aligned to 16B!
     *
     * thisproc()->tf->sp = sp;
     * ```
     *
     */

}

