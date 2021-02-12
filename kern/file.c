/* File descriptors */

#include "types.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "console.h"
#include "log.h"

struct devsw devsw[NDEV];
struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

/* Optional since BSS is zero-initialized. */
void
fileinit()
{
    /* TODO: Your code here. */
    
    initlock(&ftable.lock, "file table");
}

/* Allocate a file structure. */
struct file *
filealloc()
{
    /* TODO: Your code here. */
    
    struct file *fi;
    
    acquire(&ftable.lock);
    for (fi = ftable.file; fi < ftable.file + NFILE; fi++) {
        if (fi->ref == 0) {
            fi->ref = 1;
            release(&ftable.lock);
            return fi;
        }
    }
    release(&ftable.lock);
    return(NULL);
}

/* Increment ref count for file f. */
struct file *
filedup(struct file *f)
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    f->ref += 1;
    release(&ftable.lock);
}

/* Close file f. (Decrement ref count, close when reaches 0.) */
void
fileclose(struct file *f)
{
    /* TODO: Your code here. */

    struct file fs;
    acquire(&ftable.lock);

    f->ref -= 1;
    if (f->ref > 0) return;   
    
    f->type = FD_NONE;
    fs = *f;
    release(&ftable.lock);

    if (fs.type == FD_PIPE){
        
    }else{
        begin_op();
        iput(fs.ip);
        end_op();
    }
}

/* Get metadata about file f. */
int
filestat(struct file *f, struct stat *st)
{
    /* TODO: Your code here. */

    struct stat sts;

    if (f->type == FD_INODE){
        ilock(f->ip);
        stati(f->ip, &sts);
        iunlock(f->ip);
        st = &sts;
        return(0);
    }
    return(-1);
}

/* Read from file f. */
ssize_t
fileread(struct file *f, char *addr, ssize_t n)
{
    /* TODO: Your code here. */

    ssize_t rdn;
    if (f->type != FD_INODE)
        return(0);

    ilock(f->ip);
    rdn = readi(f->ip, addr, f->off, n);
    f->off += rdn;
    iunlock(f->ip);

    return(rdn);
}

/* Write to file f. */
ssize_t
filewrite(struct file *f, char *addr, ssize_t n)
{
    /* TODO: Your code here. */
    ssize_t fin, lim, wtn, cur;
    if (f->type != FD_INODE)
        return(0);
    
    fin = 0;
    lim = (MAXOPBLOCKS - 1 - 1 - 2) / 2 * BSIZE;

    while (fin < n)
    {
        wtn = n;
        if (n > lim)
            wtn = lim;

        begin_op();
        ilock(f->ip);
        cur = writei(f->ip, addr, f->off, wtn);
        if (cur < 0)
            break;
        fin += cur;
        f->off += cur;
        iunlock(f->ip);
        end_op();
    }

    return(fin);
    /*
     * 为了防止log溢出, inode, indirect, 上下两个不满的
     * 分多次log_write
    */
}

