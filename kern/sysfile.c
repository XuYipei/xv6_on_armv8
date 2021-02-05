//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <fcntl.h>

#include "types.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "string.h"
#include "console.h"
#include "log.h"
#include "fs.h"
#include "file.h"

struct iovec {
    void  *iov_base;    /* Starting address. */
    size_t iov_len;     /* Number of bytes to transfer. */
};

/*
 * Fetch the nth word-sized system call argument as a file descriptor
 * and return both the descriptor and the corresponding struct file.
 */
static int
argfd(int n, int64_t *pfd, struct file **pf)
{
    int64_t fd;
    struct file *f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = thisproc()->ofile[fd]) == 0)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

/*
 * Allocate a file descriptor for the given file.
 * Takes over file reference from caller on success.
 */
static int
fdalloc(struct file *f)
{
    /* TODO: Your code here. */

    int fd;
    struct proc *p;
    p = thisproc();

    for (fd = 0; fd < NOFILE; fd++){
        if (p->ofile[fd] == f)
            return(fd);
        if (!p->ofile[fd]){
            p->ofile[fd] = f;
            return(fd);
        }
    }
    return(-1);
}

int
sys_dup()
{
    /* TODO: Your code here. */

    struct file *f;
    int64_t fd;
    int result;
    if (argfd(0, &fd, &f) < 0)
        return(-1);
    result = filedup(f);
    return(result);
}

ssize_t
sys_read()
{
    /* TODO: Your code here. */

    struct file *f;
    int64_t fd;
    struct iovec *iov;
    ssize_t wtn;
    if (argfd(0, &fd, &f) < 0 || argptr(1, &iov, sizeof(struct iovec)) < 0)
        return -1;

    wtn = fileread(f, iov->iov_base, iov->iov_len);
    return(wtn);
}

ssize_t
sys_write()
{
    /* TODO: Your code here. */

    struct file *f;
    int64_t fd;
    struct iovec *iov;
    ssize_t wtn;
    if (argfd(0, &fd, &f) < 0 || argptr(1, &iov, sizeof(struct iovec)) < 0)
        return -1;

    wtn = filewrite(f, iov->iov_base, iov->iov_len);
    return(wtn);
}


ssize_t
sys_writev()
{
    /* TODO: Your code here. */

    /* Example code.
     *
     * ```
     * struct file *f;
     * int64_t fd, iovcnt;
     * struct iovec *iov, *p;
     * if (argfd(0, &fd, &f) < 0 ||
     *     argint(2, &iovcnt) < 0 ||
     *     argptr(1, &iov, iovcnt * sizeof(struct iovec)) < 0) {
     *     return -1;
     * }
     *
     * size_t tot = 0;
     * for (p = iov; p < iov + iovcnt; p++) {
     *     // in_user(p, n) checks if va [p, p+n) lies in user address space.
     *     if (!in_user(p->iov_base, p->iov_len))
     *          return -1;
     *     tot += filewrite(f, p->iov_base, p->iov_len);
     * }
     * return tot;
     * ```
     */

    struct file *f;
    int64_t fd, iovcnt;
    struct iovec *iov, *p;
    if (argfd(0, &fd, &f) < 0 || argint(2, &iovcnt) < 0) 
        return(-1);
    if (argptr(1, &iov, iovcnt * sizeof(struct iovec)) < 0)
        return(-1);

    size_t tot = 0;
    for (p = iov; p < iov + iovcnt; p++){
        tot += filewrite(f, p->iov_base, p->iov_len);
    }
    return tot;
}

int
sys_close()
{
    /* TODO: Your code here. */

    struct file *f;
    int64_t fd;
    if (argfd(0, &fd, &f) < 0)
        return(-1);
    fileclose(f);
}

int
sys_fstat()
{
    /* TODO: Your code here. */

    struct file *f;
    int64_t fd;
    int result;
    struct stat *st;
    if (argfd(0, &fd, &f) < 0 || argptr(1, &st, sizeof(struct stat)))
        return(-1);
    result = fstat(fd, st);
    return(result);
}

int
sys_fstatat()
{
    int64_t dirfd, flags;
    char *path;
    struct stat *st;

    if (argint(0, &dirfd) < 0 ||
        argstr(1, &path) < 0 ||
        argptr(2, (void *)&st, sizeof(*st)) < 0 ||
        argint(3, &flags) < 0)
        return -1;

    if (dirfd != AT_FDCWD) {
        cprintf("sys_fstatat: dirfd unimplemented\n");
        return -1;
    }
    if (flags != 0) {
        cprintf("sys_fstatat: flags unimplemented\n");
        return -1;
    }

    struct inode *ip;
    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    stati(ip, st);
    iunlockput(ip);
    end_op();

    return 0;
}

static struct inode *
create(char *path, short type, short major, short minor)
{
    /* TODO: Your code here. */

    size_t poff;
    struct file *f;
    struct inode *ind, *dp, *find;
    char name[DIRSIZ];

    dp = nameiparent(path, name);
    if (!dp) return(0);

    ilock(dp);

    find = dirlookup(dp, name, &poff);
    if (find){
        iunlock(dp);
        ilock(find);
        if (type == T_FILE)
            return(find);
        iunlock(find); 
        return(0);
    }

    ind = ialloc(dp->dev, type);
    ilock(ind);
    
    ind->major = major;
    ind->minor = minor;
    ind->type = type;
    ind->nlink = 1;
    iupdate(ind);

    if (type == T_DIR){
        dp->nlink++;
        iupdate(dp);
        dirlink(ind, ".", ind->inum);
        dirlink(ind, "..", dp->inum);
    }
    dirlink(dp, name, ind->inum);

    iunlock(ind);
    return(ind);
}

int
sys_openat()
{
    char *path;
    int64_t dirfd, fd, omode;
    struct file *f;
    struct inode *ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &omode) < 0)
        return -1;

    if (dirfd != AT_FDCWD) {
        cprintf("sys_openat: dirfd unimplemented\n");
        return -1;
    }
    if ((omode & O_LARGEFILE) == 0) {
        cprintf("sys_openat: expect O_LARGEFILE in open flags\n");
        return -1;
    }

    begin_op();
    if (omode & O_CREAT) {
        // FIXME: Support acl mode.
        ip = create(path, T_FILE, 0, 0);
        if (ip == 0) {
            end_op();
            return -1;
        }
    } else {
        if ((ip = namei(path)) == 0) {
            end_op();
            return -1;
        }
        ilock(ip);
        if (ip->type == T_DIR && omode != (O_RDONLY | O_LARGEFILE)) {
            iunlockput(ip);
            end_op();
            return -1;
        }
    }

    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    end_op();

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    return fd;
}

int
sys_mkdirat()
{
    int64_t dirfd, mode;
    char *path;
    struct inode *ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &mode) < 0)
        return -1;
    if (dirfd != AT_FDCWD) {
        cprintf("sys_mkdirat: dirfd unimplemented\n");
        return -1;
    }
    if (mode != 0) {
        cprintf("sys_mkdirat: mode unimplemented\n");
        return -1;
    }

    begin_op();
    if ((ip = create(path, T_DIR, 0, 0)) == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int
sys_mknodat()
{
    struct inode *ip;
    char *path;
    int64_t dirfd, major, minor;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &major) < 0 || argint(3, &minor))
        return -1;

    if (dirfd != AT_FDCWD) {
        cprintf("sys_mknodat: dirfd unimplemented\n");
        return -1;
    }
    cprintf("mknodat: path '%s', major:minor %d:%d\n", path, major, minor);

    begin_op();
    if((ip = create(path, T_DEV, major, minor)) == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int
sys_chdir()
{
    char *path;
    struct inode *ip;
    struct proc *curproc = thisproc();
  
    begin_op();
    if (argstr(0, &path) < 0 || (ip = namei(path)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    if (ip->type != T_DIR) {
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    iput(curproc->cwd);
    end_op();
    curproc->cwd = ip;
    return 0;
}

int
sys_exec()
{
    /* TODO: Your code here. */
}

