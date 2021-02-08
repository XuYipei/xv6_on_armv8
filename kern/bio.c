/* Buffer cache.
 *
 * The buffer cache is a linked list of buf structures holding
 * cached copies of disk block contents.  Caching disk blocks
 * in memory reduces the number of disk reads and also provides
 * a synchronization point for disk blocks used by multiple processes.
 *
 * Interface:
 * * To get a buffer for a particular disk block, call bread.
 * * After changing buffer data, call bwrite to write it to disk.
 * * When done with the buffer, call brelse.
 * * Do not use the buffer after calling brelse.
 * * Only one process at a time can use a buffer,
 *     so do not keep them longer than necessary.
 *
 * The implementation uses two state flags internally:
 * * B_VALID: the buffer data has been read from the disk.
 * * B_DIRTY: the buffer data has been modified
 *     and needs to be written to disk.
 */

#include "spinlock.h"
#include "sleeplock.h"
#include "buf.h"
#include "console.h"
#include "sd.h"
#include "fs.h"

#include "list.h" 

struct {
    struct spinlock lock;
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // head.next is most recently used.
    struct buf head;
} bcache;

/* Initialize the cache list and locks. */
void
binit()
{
    /* TODO: Your code here. */

    initlock(&bcache.lock, "bcache lock");
    list_init(&bcache.head.blist);
    
    struct buf *bf;
    for (bf = bcache.buf; bf < bcache.buf + NBUF; bf++){
        list_push_back(&bcache.head.blist, &bf->blist);
        initsleeplock(&bf->lock, "bcache buf lock");
    }
}

/*
 * Look through buffer cache for block on device dev.
 * If not found, allocate a buffer.
 * In either case, return locked buffer.
 */
static struct buf *
bget(uint32_t dev, uint32_t blockno)
{
    /* TODO: Your code here. */
    
    acquire(&bcache.lock);

    struct buf *bf;
    struct list_head *it;

    for (it = bcache.head.blist.next; it != &bcache.head.blist ; it = it->next){
        bf = (struct buf *)container_of(&it, struct buf, blist);
        if (bf->dev != dev || bf->blockno != blockno)
            continue;
        bf->refcnt++;
        release(&bcache.lock);
        acquiresleep(&bf->lock);
        return(bf);
    }

    for (it = bcache.head.blist.prev; it != &bcache.head.blist; it = it->prev){
        bf = (struct buf *)container_of(&it, struct buf, blist);
        if (bf->refcnt == 0)
            continue;
        bf->flags = 0;
        bf->refcnt = 1;
        bf->dev = dev;
        bf->blockno = blockno;
        release(&bcache.lock);
        acquiresleep(&bf->lock);
        return(bf);
    }
    panic("No such buffer!");
}

/* Return a locked buf with the contents of the indicated block. */
struct buf *
bread(uint32_t dev, uint32_t blockno)
{
    /* TODO: Your code here. */
    struct buf* bf;
    bf = bget(dev, blockno);
    if (!(bf->flags & B_VALID)){
        sdrw(bf);
        bf->flags |= B_VALID;
    }
    return(bf);
}

/* Write b's contents to disk. Must be locked. */
void
bwrite(struct buf *b)
{
    /* TODO: Your code here. */
    if (!holdingsleep(&b->lock))
        panic("Write buffer isn't locked");
    b->flags |= B_DIRTY;
    sdrw(b);
}

/*
 * Release a locked buffer.
 * Move to the head of the MRU list.
 */
void
brelse(struct buf *b)
{
    /* TODO: Your code here. */
    if (!holdingsleep(&b->lock))
       panic("Release buffer isn't locked");
    
    releasesleep(&b->lock);
    acquire(&bcache.lock);

    b->refcnt--;
    list_delete(&b->blist);
    list_push_front(&bcache.head.blist, &b->blist);

    release(&bcache.lock);
}

void 
bpin(struct buf *b)
{
    acquire(&bcache.lock);
    b->refcnt += 1;
    release(&bcache.lock);
}

void
bunpin(struct buf *b)
{
    acquire(&bcache.lock);
    b->refcnt -= 1;
    release(&bcache.lock);
}
