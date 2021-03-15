#include "memlayout.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"

int copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len) {
  memmove(dst,(void*)(srcva),len);
  return 0;
}
