// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// lab6 锁和引用计数
struct ref_stru {
    struct spinlock lock;
    int cnt[PHYSTOP / PGSIZE];
} ref;


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
      ref.cnt[(uint64) p / PGSIZE] = 1;
      kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // lab6 只在引用计数为0的时候才释放内存,但是应该只对于cow映射的页表做检查
  if(--ref.cnt[(uint64)pa / PGSIZE] == 0) {

        r = (struct run*)pa;

        // Fill with junk to catch dangling refs.
        memset(pa, 1, PGSIZE);

        acquire(&kmem.lock);
        r->next = kmem.freelist;
        kmem.freelist = r;
        release(&kmem.lock);
    }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
      kmem.freelist = r->next;
      acquire(&ref.lock);
      ref.cnt[(uint64)r / PGSIZE] = 1;  // 将引用计数初始化为1
      release(&ref.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}

// lab6 分配cow页面
void* cowalloc(pagetable_t pagetable, uint64 va) {
    pte_t* pte = walk(pagetable, va, 0); //va来自r_stval()
    uint64 pa = PTE2PA(*pte);  //需要进行拷贝的物理地址（父进程）
    char* mem;
    if((mem = kalloc()) == 0)
        return 0;
    memmove(mem, (char*)pa, PGSIZE); //把父进程的物理内存拷贝给子进程

    // remap问题
    *pte &= ~PTE_V;
//    *pte = 0;
//    uvmunmap(pagetable,va,1,0);

    // 为子进程新的物理内存建立映射
    if(mappages(pagetable, va, PGSIZE, (uint64)mem, (PTE_FLAGS(*pte) | PTE_W) & ~PTE_COW) != 0) {
        kfree(mem);
        return 0;
    }
    // 由于子进程不再指向父进程的物理内存，引用计数放在kfree中减1
    kfree((char*)PGROUNDDOWN(pa));
    return mem;
}


// lab6 判断是否为cow页面
int iscowpage(pagetable_t pagetable, uint64 va) {
    if(va >= MAXVA)
        return -1;
    pte_t* pte = walk(pagetable, va, 0);
    if(pte == 0)
        return -1;
    if((*pte & PTE_V) == 0)
        return -1;
    return (*pte & PTE_COW ? 0 : -1);
}

int kaddrefcnt(void* pa) {
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        return -1;
    acquire(&ref.lock);
    ++ref.cnt[(uint64)pa / PGSIZE];
    release(&ref.lock);
    return 0;
}