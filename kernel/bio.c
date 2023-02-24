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
#define HASH(id) (id % NBUCKET)

//struct {
//  struct spinlock lock;
//  struct buf buf[NBUF];
//
//  // Linked list of all buffers, through prev/next.
//  // Sorted by how recently the buffer was used.
//  // head.next is most recent, head.prev is least.
//  struct buf head;
//} bcache;

struct hashbuf {
    struct buf head;
    struct spinlock lock;
};

struct {
    struct buf buf[NBUF];
    struct hashbuf buckets[NBUCKET];  // 散列桶
} bcache;


void
binit(void)
{
    struct buf *b;
    char lockname[16];
    for(int i = 0; i < NBUCKET; ++i) {
        // 初始化散列桶的自旋锁
        snprintf(lockname, sizeof(lockname), "bcache_%d", i);
        initlock(&bcache.buckets[i].lock, "bcache");

        // 初始化散列桶的头节点
        bcache.buckets[i].head.prev = &bcache.buckets[i].head;
        bcache.buckets[i].head.next = &bcache.buckets[i].head;
    }
    for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
        // 利用头插法初始化缓冲区列表,全部放到散列桶0上
        b->next = bcache.buckets[0].head.next;
        b->prev = &bcache.buckets[0].head;
        initsleeplock(&b->lock, "buffer");
        bcache.buckets[0].head.next->prev = b;
        bcache.buckets[0].head.next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketno = HASH(blockno);
  acquire(&bcache.buckets[bucketno].lock);

  // 寻找空的buf
    for (b = bcache.buckets[bucketno].head.next; b != &bcache.buckets[bucketno].head; b = b->next) {
        if(b->dev == dev && b->blockno == blockno){
            b->refcnt++;
            release(&bcache.buckets[bucketno].lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    // 未在当前桶找到空的batch
    // 首先在当前桶内尝试重用缓存块,寻找timestamp最小的buf
    struct buf *target = 0;
    for(b = bcache.buckets[bucketno].head.next; b != &bcache.buckets[bucketno].head; b = b->next) {
        if (b->refcnt == 0 && (target == 0 || b->timestamp < target->timestamp)) {
            target = b;
        }
    }
    // 在当前桶内找到空闲且未使用时间最久的buf
    if (target){
        target->dev = dev;
        target->blockno = blockno;
        target->valid = 0;
        target->refcnt = 1;
        release(&bcache.buckets[bucketno].lock);
        acquiresleep(&target->lock);
        return target;
    }

    // 桶内没有闲置的缓存块则去其他桶偷一块
    // 查找未使用时间最长的buf，此处的LRU只是桶内最优，而不是全局最优
    for (int i = 0; i < NBUCKET; ++i) {
        if (i == bucketno) // 自己的桶已经找过了
            continue;
        if(!holding(&bcache.buckets[i].lock)) // 防止死锁，但是可能导致找不到空闲缓存
            acquire(&bcache.buckets[i].lock);
        else
            continue;
        target = 0;
        for (b = bcache.buckets[i].head.next; b != &bcache.buckets[i].head; b = b->next) {
            if (b->refcnt == 0 && (target == 0 || b->timestamp < target->timestamp)) {
                target = b;
            }
        }
        if (target){
            // 把节点偷过来
            // 原来的桶上解除连接
            target->next->prev = target->prev;
            target->prev->next = target->next;
            release(&bcache.buckets[i].lock);

            // 头插法插入当前桶
            target->next = bcache.buckets[bucketno].head.next;
            target->prev = &bcache.buckets[bucketno].head;
            bcache.buckets[bucketno].head.next->prev = target;
            bcache.buckets[bucketno].head.next = target;

            target->dev = dev;
            target->blockno = blockno;
            target->valid = 0;
            target->refcnt = 1;
            release(&bcache.buckets[bucketno].lock);
            acquiresleep(&target->lock);
            return target;
        }
        release(&bcache.buckets[i].lock);
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

  int bucketno = HASH(b->blockno);
  releasesleep(&b->lock);

    // 把buf释放到对应的桶内
    acquire(&bcache.buckets[bucketno].lock);
    b->timestamp = ticks;
    b->refcnt--;
    release(&bcache.buckets[bucketno].lock);
}

void
bpin(struct buf *b) {
    int bucketno = HASH(b->blockno);
    acquire(&bcache.buckets[bucketno].lock);
    b->refcnt++;
    release(&bcache.buckets[bucketno].lock);
}

void
bunpin(struct buf *b) {
    int bucketno = HASH(b->blockno);
    acquire(&bcache.buckets[bucketno].lock);
    b->refcnt--;
    release(&bcache.buckets[bucketno].lock);
}


