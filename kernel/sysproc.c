#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_mmap(void){
  uint64 length;
  int prot;
  int flags;
  int fd;
  uint64 offset;

  struct file * f;

  argaddr(1,&length);
  argint(2, &prot);
  argint(3, &flags);
  argint(4, &fd);
  argaddr(5,&offset);


  // find a empty vma
  struct proc * p = myproc();

  if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0){
    printf("invalid fd for this process\n");
    return -1;
  }

  int r = prot & PROT_READ ? 1:0;
  int w = prot & PROT_WRITE ? 1:0;

  if(r != f->readable || (w != f->writable && flags != MAP_PRIVATE)){
    printf("%d %d %d %d\n",r,f->readable,w,f->writable);
    return -1;
  }
  // check prot according to file access

  struct vma * v = 0;
  uint64 next_begin = 0;

  acquire(&p->lock);
  if(p->vmacount >= NVMA){
    release(&p->lock);
    return -1;
  }

  // In this lab, we do not need an allocator for a general vma allocation.
  // Just allocate anywhere you like
  if(p->vmacount == 0){
    next_begin = PGROUNDDOWN(TRAPFRAME - 10000*PGSIZE);
  }else{
    next_begin = PGROUNDDOWN(((uint64)p->vmas[p->vmacount - 1].start - length));
  }
  v = &p->vmas[p->vmacount]; // find an empty vma
  p->vmacount++; 
  release(&p->lock);

  if(!v){
    // cannot find va for mapping
    return -1;
  }
  if(v->file){
    panic("not an empty vma\n");
  }
  printf("sys_mmap: length: %d, prot: %d, flags: %d,fd: %d,offset: %d, map to: %p\n",length,prot,flags,fd,offset,next_begin);



  filedup(f);  // add ref

  v->file = f;
  v->start = (void*)next_begin;
  v->offset = offset;
  v->len = length;
  v->prot = prot;
  v->flags = flags;

 // growproc((uint64)v->start + length);

  return (uint64)v->start;
}

uint64
sys_munmap(void){
  uint64 addr;
  uint64 len;
  argaddr(0,&addr);
  argaddr(1,&len);
  struct vma * v = 0;
  struct proc * p = myproc();
  for(int i = 0;i<p->vmacount;i++){
    struct vma * pv = &p->vmas[i];
    if(addr >= (uint64)pv->start && addr<(uint64)pv->start + pv->len){
      v = pv;
      break;
    }
  }
  if(!v){
    printf("cannot find corresponding vma\n");
    return -1;
  }
  //
  // Assuming that the addr to be unmapped will either at the start,
  // or at the end, or the whole region, but not punch a hole in the
  // middle of a region (it will generate a new vma)
  //
  uint64 newstart = 0;
  uint64 newlen = 0;
  if(addr == (uint64)v->start && len == v->len){ // whole
    // unmmap the whole region
    newstart = 0;
    newlen = 0;
    uvmunmap(p->pagetable,PGROUNDDOWN((uint64)v->start),len/PGSIZE,1);
  }else if(addr == (uint64)v->start && len <= v->len){ // start
    newstart = (uint64)v->start + len;
    newlen = v->len -len;
    uvmunmap(p->pagetable,PGROUNDDOWN((uint64)v->start),len/PGSIZE,1);
  }else if(addr +len == (uint64)v->start + v->len){    // end
    newstart = (uint64)v->start;
    newlen = v->len - len;
    uvmunmap(p->pagetable,PGROUNDDOWN((uint64)v->start + len),len/PGSIZE,2);
  }else{
    printf("invalid unmap region: %p, %d in %p, %d\n",addr,len,v->start,v->len);
    return -1;
  }
  printf("unmap region: %p, %d in %p, %d\n",addr,len,v->start,v->len);

  if(v->flags == MAP_SHARED){
    // write back to the file
    filewrite(v->file,addr,len);
  }

  if(newlen == 0){
    fileclose(v->file);
    v->file = 0;
  }
  v->start = (void*)newstart;
  v->len = newlen;
  return 0;
}
