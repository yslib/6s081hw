// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define NBUCKET 13

struct bcache_t{
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

static struct bcache_t hashtable[NBUCKET];


extern uint ticks;

void
init_bchache(struct bcache_t * bch){

  struct buf *b;
  // Create linked list of buffers
  initlock(&bch->lock, "bcache.bucket");
  bch->head.prev = &bch->head;
  bch->head.next = &bch->head;
  for(b = bch->buf; b < bch->buf+NBUF; b++){
    b->next = bch->head.next;
    b->prev = &bch->head;
    initsleeplock(&b->lock, "buffer");
    bch->head.next->prev = b;
    bch->head.next = b;
    b->bcache = bch;
    b->timestamp = ticks;
  }
}

struct bcache_t *
hash(int dev,int blockno){
  int hash_index = ((dev * 10) + blockno)%NBUCKET;
  //printf("hashed index: %d\n",hash_index);
  return &hashtable[hash_index];
}

void
binit(void)
{
  init_bchache(&bcache);

  for(int i = 0 ; i < NBUCKET;i++){
    init_bchache(&hashtable[i]);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  //struct bcache_t * bch = &bcache;
  struct bcache_t * bch = hash(dev,blockno);

  acquire(&bch->lock);

  // Is the block already cached?
  for(b = bch->head.next; b != &bch->head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bch->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  //
  
  // for(b = bch->head.prev; b != &bch->head; b = b->prev){
    // if(b->refcnt == 0) {
      // b->dev = dev;
      // b->blockno = blockno;
      // b->valid = 0;
      // b->refcnt = 1;
      // release(&bch->lock);
      // acquiresleep(&b->lock);
      // return b;
    // }
  // }

  uint min_tick = -1;
  int ind = -1;

  for(int i = 0 ; i< NBUF;i++){
    uint cur_tick = bch->buf[i].timestamp;
    if(b->refcnt ==0 && min_tick > cur_tick){
      min_tick = cur_tick;
      ind = i;
    }
  }

  if(ind != -1){
    struct buf * b = &bch->buf[ind];
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->timestamp = ticks;
    release(&bch->lock);
    acquiresleep(&b->lock);
    return b;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  //struct bcache_t * bch = b->bcache;

  //acquire(&bch->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.

    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bch->head.next;
    // b->prev = &bch->head;
    // bch->head.next->prev = b;
    // bch->head.next = b;
  }
  
  //release(&bch->lock);
}

void
bpin(struct buf *b) {
  struct bcache_t * bch = b->bcache;
  acquire(&bch->lock);
  b->refcnt++;
  release(&bch->lock);
}

void
bunpin(struct buf *b) {
  struct bcache_t * bch = b->bcache;
  acquire(&bch->lock);
  b->refcnt--;
  release(&bch->lock);
}


