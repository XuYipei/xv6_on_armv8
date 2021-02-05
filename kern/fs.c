/*
 * File system implementation.  Five layers:
 *   + Blocks: allocator for raw disk blocks.
 *   + Log: crash recovery for multi-step updates.
 *   + Files: inode allocator, reading, writing, metadata.
 *   + Directories: inode with special contents (list of other inodes!)
 *   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
 *
 * This file contains the low-level file system manipulation
 * routines.  The (higher-level) system call implementations
 * are in sysfile.c.
 */

#include "types.h"
#include "mmu.h"
#include "proc.h"
#include "string.h"
#include "console.h"

#include "spinlock.h"
#include "sleeplock.h"

#include "buf.h"
#include "log.h"
#include "file.h"


#define min(a, b) ((a) < (b) ? (a) : (b))

static void itrunc(struct inode*);

// There should be one superblock per disk device,
// but we run with only one device.
struct superblock sb; 

/* Read the super block. */
void
readsb(int dev, struct superblock *sb)
{
    /* TODO: Your code here. */

    struct buf *bf = bread(dev, 0);
    memcpy(sb, bf->data, sizeof(struct superblock));
    brelse(bf);
}

/* Zero a block. */
static void
bzero(int dev, int bno)
{
    /* TODO: Your code here. */

    struct buf *bf = bread(dev, bno);
    memset(bf->data, 0, sizeof(bf->data));
    log_write(bf);
    brelse(bf);
}

/* Blocks. */

/* Allocate a zeroed disk block. */
static uint32_t
balloc(uint32_t dev)
{
    /* TODO: Your code here. */

    int i, j, k;
    struct buf *bf;
    for (i = 0; i < sb.size; i += BPB){
        bf = bread(dev, BBLOCK(i, sb));
        for (j = 0; j < BPB; j++){
            k = 1 << (j % 8);
            if (!(bf->data[j / 8] & k)){
                bf->data[j / 8] |= k;
                log_write(bf);
                brelse(bf);
                bzero(dev, i + j);
                return(i + j);
            }
        }
        brelse(bf);
    }
}

/* Free a disk block. */
static void
bfree(int dev, uint32_t b)
{
    /* TODO: Your code here. */

    int i, j, k;
    struct buf *bf;
    i = b / BPB;
    j = b % BPB / 8;
    k = 1 << (b % BPB % 8);

    bf = bread(dev, BBLOCK(i, sb));
    if (bf->data[j] & k)
        panic("Free block has been freed");
    bf->data[j] ^= k;
    log_write(bf);
    brelse(bf);
    // bzero(dev, b);
}

/* Inodes.
 *
 * An inode describes a single unnamed file.
 * The inode disk structure holds metadata: the file's type,
 * its size, the number of links referring to it, and the
 * list of blocks holding the file's content.
 *
 * The inodes are laid out sequentially on disk at
 * sb.startinode. Each inode has a number, indicating its
 * position on the disk.
 *
 * The kernel keeps a cache of in-use inodes in memory
 * to provide a place for synchronizing access
 * to inodes used by multiple processes. The cached
 * inodes include book-keeping information that is
 * not stored on disk: ip->ref and ip->valid.
 *
 * An inode and its in-memory representation go through a
 * sequence of states before they can be used by the
 * rest of the file system code.
 *
 * * Allocation: an inode is allocated if its type (on disk)
 *   is non-zero. ialloc() allocates, and iput() frees if
 *   the reference and link counts have fallen to zero.
 *
 * * Referencing in cache: an entry in the inode cache
 *   is free if ip->ref is zero. Otherwise ip->ref tracks
 *   the number of in-memory pointers to the entry (open
 *   files and current directories). iget() finds or
 *   creates a cache entry and increments its ref; iput()
 *   decrements ref.
 *
 * * Valid: the information (type, size, &c) in an inode
 *   cache entry is only correct when ip->valid is 1.
 *   ilock() reads the inode from
 *   the disk and sets ip->valid, while iput() clears
 *   ip->valid if ip->ref has fallen to zero.
 *
 * * Locked: file system code may only examine and modify
 *   the information in an inode and its content if it
 *   has first locked the inode.
 *
 * Thus a typical sequence is:
 *   ip = iget(dev, inum)
 *   ilock(ip)
 *   ... examine and modify ip->xxx ...
 *   iunlock(ip)
 *   iput(ip)
 *
 * ilock() is separate from iget() so that system calls can
 * get a long-term reference to an inode (as for an open file)
 * and only lock it for short periods (e.g., in read()).
 * The separation also helps avoid deadlock and races during
 * pathname lookup. iget() increments ip->ref so that the inode
 * stays cached and pointers to it remain valid.
 *
 * Many internal file system functions expect the caller to
 * have locked the inodes involved; this lets callers create
 * multi-step atomic operations.
 *
 * The icache.lock spin-lock protects the allocation of icache
 * entries. Since ip->ref indicates whether an entry is free,
 * and ip->dev and ip->inum indicate which i-node an entry
 * holds, one must hold icache.lock while using any of those fields.
 *
 * An ip->lock sleep-lock protects all ip-> fields other than ref,
 * dev, and inum.  One must hold ip->lock in order to
 * read or write that inode's ip->valid, ip->size, ip->type, &c.
 */

struct {
  struct spinlock lock;
  struct inode inode[NINODE];
} icache;

void
iinit(int dev)
{
    /* TODO: Your code here. */
}

static struct inode* iget(uint32_t dev, uint32_t inum);

/* Allocate an inode on device dev.
 *
 * Mark it as allocated by giving it type type.
 * Returns an unlocked but allocated and referenced inode.
 */
struct inode*
ialloc(uint32_t dev, short type)
{
    /* TODO: Your code here. */
    int i, j, k;
    struct buf *bf;
    struct dinode *ind;
    for (i = 0; i < sb.ninodes; i+=IPB){
        bf = bread(dev, IBLOCK(i, sb));
        for (j = 0; j < IPB; j++){
            ind = ((struct dinode *)(bf->data)) + j;
            if (ind->type) continue;
            ind->type = type;
            log_write(bf);
            brelse(bf);
            return(iget(dev, i));
        }
        brelse(bf);
    }
    panic("Ialloc cannot alloc inode");
}

/* Copy a modified in-memory inode to disk.
 *
 * Must be called after every change to an ip->xxx field
 * that lives on disk, since i-node cache is write-through.
 * Caller must hold ip->lock.
 */
void
iupdate(struct inode *ip)
{
    /* TODO: Your code here. */
    struct buf *bf;
    struct dinode *didata;
    bf = bread(ip->dev, IBLOCK(ip->inum, sb));
    didata = ((struct dinode *)(bf->data)) + ip->inum % IPB;
    didata->type = ip->type;
    didata->major = ip->major;
    didata->minor = ip->minor;
    didata->nlink = ip->nlink;
    didata->size = ip->size;
    memcpy(didata->addrs, ip->addrs, sizeof(didata->addrs));
    log_write(bf);
    brelse(bf);
}

/*
 * Find the inode with number inum on device dev
 * and return the in-memory copy. Does not lock
 * the inode and does not read it from disk.
 */
static struct inode*
iget(uint32_t dev, uint32_t inum)
{
    /* TODO: Your code here. */

    int i, j, k;
    struct inode *ind = 0, *emp = 0;
    acquire(&icache.lock);
    for (ind = 0; ind < icache.inode + NINODE; ind++){
        if (ind->inum == inum && ind->ref > 0 && ind->dev == dev){
            ind->ref++;
            release(&icache.lock);
            return ind;
        }else{
            if (ind->ref == 0 && emp == 0)
                emp = ind;
        }
    }

    if (emp == 0)
        panic("Cannot alloc a inode");

    emp->dev = dev;
    emp->inum = inum;
    emp->ref = 1;
    emp->size = 0;
    emp->valid = 0;
    release(&icache.lock);
    return(emp);
}

/* 
 * Increment reference count for ip.
 * Returns ip to enable ip = idup(ip1) idiom.
 */
struct inode*
idup(struct inode *ip)
{
    /* TODO: Your code here. */

    acquire(&icache.lock);
    ip->ref += 1;
    release(&icache.lock);
    return(ip);
}

/* 
 * Lock the given inode.
 * Reads the inode from disk if necessary.
 */
void
ilock(struct inode *ip)
{
    /* TODO: Your code here. */

    struct buf *bf;
    struct dinode *didata;
    acquiresleep(&ip->lock);
    
    if (!ip->valid){
        bf = bread(ip->dev, IBLOCK(ip->inum, sb));
        didata = ((struct dinode *)(bf->data)) + ip->inum % IPB;
        ip->type = didata->type;
        ip->major = didata->major;
        ip->minor = didata->minor;
        ip->nlink = didata->nlink;
        ip->size = didata->size;
        memcpy(ip->addrs, didata->addrs, sizeof(ip->addrs));
        ip->valid = 1;
    }
}

/* Unlock the given inode. */
void
iunlock(struct inode *ip)
{
    /* TODO: Your code here. */

    releasesleep(&ip->lock);
}

/* Drop a reference to an in-memory inode.
 *
 * If that was the last reference, the inode cache entry can
 * be recycled.
 * If that was the last reference and the inode has no links
 * to it, free the inode (and its content) on disk.
 * All calls to iput() must be inside a transaction in
 * case it has to free the inode.
 */
void
iput(struct inode *ip)
{
    /* TODO: Your code here. */
    
    acquire(&icache.lock);
    if (ip->ref == 1 && ip->valid && ip->nlink == 0){
        acquiresleep(&ip->lock);
        release(&icache.lock);

        itrunc(ip);
        ip->type = 0;
        iupdate(ip);
        ip->valid = 0;

        releasesleep(&ip->lock);
        acquire(&icache.lock);
    }
    ip->ref -= 1;
    release(&icache.lock);
    /* 
     * ref 先不置为 0, 等所有操作结束后再置 0, 防止该 inode 在 ref = 0 后被另外进程的 iget 修改
     */
}

/* Common idiom: unlock, then put. */
void
iunlockput(struct inode *ip)
{
    /* TODO: Your code here. */
    iunlock(ip);
    iput(ip);
}

/* Inode content
 *
 * The content (data) associated with each inode is stored
 * in blocks on the disk. The first NDIRECT block numbers
 * are listed in ip->addrs[].  The next NINDIRECT blocks are
 * listed in block ip->addrs[NDIRECT].
 *
 * Return the disk block address of the nth block in inode ip.
 * If there is no such block, bmap allocates one.
 */
static uint32_t
bmap(struct inode *ip, uint32_t bn)
{
    /* TODO: Your code here. */

    uint32_t *addrs;
    struct buf *bf;
    if (bn < NDIRECT){
        if (ip->addrs[bn] == 0)
            ip->addrs[bn] = balloc(ip->dev);
        return(ip->addrs[bn]);
    }
    bn -= NDIRECT;
    if (bn < NINDIRECT){
        if (ip->addrs[NDIRECT] == 0)
            ip->addrs[NDIRECT] = balloc(ip->dev);
        bf = bread(ip->dev, ip->addrs[NDIRECT]);
        addrs = (uint32_t *)bf->data;
        if (addrs[bn] == 0)
            addrs[bn] = balloc(ip->dev);
        brelse(bf);
        return(addrs[bn]);
    }
    panic("Cannot bmap such block");
}

/* Truncate inode (discard contents).
 *
 * Only called when the inode has no links
 * to it (no directory entries referring to it)
 * and has no in-memory reference to it (is
 * not an open file or current directory).
 */
static void
itrunc(struct inode *ip)
{
    /* TODO: Your code here. */

    uint32_t i, *addrs;
    struct buf *bf;
    for (i = 0; i < NDIRECT; i++){
        if (!ip->addrs[i]) continue;
        bfree(ip->dev, ip->addrs[i]);
    }

    if (ip->addrs[NDIRECT]){
        bf = bread(ip->dev, ip->addrs[NDIRECT]);
        addrs = (uint32_t *)bf->data;
        for (i = 0; i < NINDIRECT; i++){
            if (!addrs[i]) continue;
            bfree(ip->dev, addrs[i]);
        }
        brelse(bf);
        bfree(ip->dev, ip->addrs[NDIRECT]);
    }

    ip->size = 0;
    iupdate(ip);
}

/*
 * Copy stat information from inode.
 * Caller must hold ip->lock.
 */
void
stati(struct inode *ip, struct stat *st)
{
    // FIXME: Support other fields in stat.
    st->st_dev = ip->dev;
    st->st_ino = ip->inum;
    st->st_nlink = ip->nlink;
    st->st_size = ip->size;

    switch (ip->type) {
    case T_FILE:
        st->st_mode = S_IFREG;
        break;
    case T_DIR:
        st->st_mode = S_IFDIR;
        break;
    case T_DEV:
        st->st_mode = 0;
        break;
    default:
        panic("unexpected stat type %d. ", ip->type);
    }
}

/*
 * Read data from inode.
 * Caller must hold ip->lock.
 */
ssize_t
readi(struct inode *ip, char *dst, size_t off, size_t n)
{
    size_t tot, m;
    struct buf *bp;

    if (ip->type == T_DEV) {
        if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
            return -1;
        return devsw[ip->major].read(ip, dst, n);
    }

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > ip->size)
        n = ip->size - off;

    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        bp = bread(ip->dev, bmap(ip, off/BSIZE));
        m = min(n - tot, BSIZE - off%BSIZE);
        memmove(dst, bp->data + off%BSIZE, m);
        brelse(bp);
    }
    return n;
}

/*
 * Write data to inode.
 * Caller must hold ip->lock.
 */
ssize_t
writei(struct inode *ip, char *src, size_t off, size_t n)
{
    size_t tot, m;
    struct buf *bp;

    if (ip->type == T_DEV) {
        if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
            return -1;
        return devsw[ip->major].write(ip, src, n);
    }

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE*BSIZE)
        return -1;

    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        bp = bread(ip->dev, bmap(ip, off/BSIZE));
        m = min(n - tot, BSIZE - off%BSIZE);
        memmove(bp->data + off%BSIZE, src, m);
        log_write(bp);
        brelse(bp);
    }

    if (n > 0 && off > ip->size) {
        ip->size = off;
        iupdate(ip);
    }
    return n;
}

/* Directories. */

int
namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/*
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entr,m ,blmky.
 */
struct inode*
dirlookup(struct inode *dp, char *name, size_t *poff)
{
    size_t off, inum;
    struct dirent de;

    if(dp->type != T_DIR)
        panic("dirlookup not DIR");

    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0)
            continue;
        if (namecmp(name, de.name) == 0) {
            // entry matches path element
            if (poff)
                *poff = off;
            inum = de.inum;
            return iget(dp->dev, inum);
        }
    }
    return 0;
}

/* Write a new directory entry (name, inum) into the directory dp. */
int
dirlink(struct inode *dp, char *name, uint32_t inum)
{
    ssize_t off;
    struct dirent de;
    struct inode *ip;

    /* Check that name is not present. */
    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iput(ip);
        return -1;
    }

    /* Look for an empty dirent. */
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        if(de.inum == 0)
            break;
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if (writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
        panic("dirlink");

    return 0;
}


/* Paths. */

/* Copy the next path element from path into name.
 *
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path=='\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples:
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */
static char*
skipelem(char *path, char *name)
{
    char *s;
    int len;

    while (*path == '/')
        path++;
    if (*path == 0)
        return 0;
    s = path;
    while (*path != '/' && *path != 0)
        path++;
    len = path - s;
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while (*path == '/')
        path++;
    return path;
}

/* Look up and return the inode for a path name.
 * 
 * If parent != 0, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Must be called inside a transaction since it calls iput().
 */
static struct inode *
namex(char *path, int nameiparent, char *name)
{
    struct inode *ip, *next;

    if (*path == '/')
        ip = iget(ROOTDEV, ROOTINO);
    else
        ip = idup(thiscpu->proc->cwd);

    while ((path = skipelem(path, name)) != 0) {
        ilock(ip);
        if (ip->type != T_DIR) {
            iunlockput(ip);
            return 0;
        }
        if (nameiparent && *path == '\0') {
            // Stop one level early.
            iunlock(ip);
            return ip;
        }
        if ((next = dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            return 0;
        }
        iunlockput(ip);
        ip = next;
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}

struct inode *
namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, 0, name);
}

struct inode *
nameiparent(char *path, char *name)
{
    return namex(path, 1, name);
}
