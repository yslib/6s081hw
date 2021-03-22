// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "memlayout.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

extern int pmrefcnt[MAXPHYPG];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void kinit() {
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
  printf("kinit: %d ph blocks in total\n", MAXPHYPG);
  for (int i = 0; i < MAXPHYPG; i++) {
    pmrefcnt[i] = 0;
  }
}

void kfreeinit(void *pa) {
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfreeinit");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void freerange(void *pa_start, void *pa_end) {
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfreeinit(p);
}


// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.

  r = (struct run *)pa;

  acquire(&kmem.lock);
  uint64 index = PHYIDX(PGROUNDDOWN((uint64)r));
  if(index >= MINPHYPG && index < MAXPHYPG){
    // only create ref cnt for allocatable physical memory
    if (--pmrefcnt[index] > 0) {
      release(&kmem.lock);
      return;
    }
  }
  memset(pa, 1, PGSIZE);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  //printf("kfree:%p\n",r);
}



// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;

  if(r){
    uint64 index = PHYIDX(PGROUNDDOWN((uint64)r));
    if(index >= MINPHYPG && index < MAXPHYPG){
      // only create ref cnt for allocatable physical memory
      if (pmrefcnt[index] != 0) {
        printf("alloc dirty phm:%p (%p - %p) %d %d\n",r,KERNBASE,PHYSTOP,index,pmrefcnt[index]);
        exit(-1);
      }
      pmrefcnt[index] = 1;
    }
  }

  release(&kmem.lock);
  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

int kgetref(const void * pa){

  uint64 index = PHYIDX(PGROUNDDOWN(PTE2PA((uint64)pa)));
  if(index >= MINPHYPG && index < MAXPHYPG){
    // only create ref cnt for allocatable physical memory
  int ref;
  acquire(&kmem.lock);
  //uint64 index = PHYIDX(PGROUNDDOWN((uint64)pa));

  //uint64 index = PHYIDX((uint64)pa);
  ref = pmrefcnt[index];
  release(&kmem.lock);
  return ref;
  }
  return -1;
}

void kaddref(void *pa) {
  uint64 index = PHYIDX(PGROUNDDOWN(PTE2PA((uint64)pa)));
  if(index >= MINPHYPG && index < MAXPHYPG){
    // only create ref cnt for allocatable physical memory
    acquire(&kmem.lock);
    //uint64 index = PHYIDX((uint64)pa);
    //printf("kalloc: add ref for %p (%d) \n",pa, index);
    if(pmrefcnt[index] <= 0){
      panic("invalid ref\n");
    }
    pmrefcnt[index]++;
    release(&kmem.lock);
    }
}

