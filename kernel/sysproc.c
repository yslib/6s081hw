#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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

  struct vma * v = 0;
  uint64 next_begin = 0;

  acquire(&p->lock);
  if(p->vmacount >= NVMA){
    release(&p->lock);
    return -1;
  }
  if(p->vmacount == 0){
    next_begin = PGROUNDDOWN(TRAPFRAME - length);
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

  return (uint64)v->start;
}

uint64
sys_munmap(void){
  return 0;
}
