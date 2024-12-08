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

struct ref_stru {
  struct spinlock lock;
  int cnt[PHYSTOP / PGSIZE];  // 引用计数
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
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref.cnt[(uint64)p / PGSIZE] = 1;
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

  acquire(&ref.lock);
  if(--ref.cnt[(uint64)pa/PGSIZE] == 0){
    release(&ref.lock);

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  } else{
    release(&ref.lock);
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
  if(r){
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

int
is_cowpage(pagetable_t pagetable, uint64 fault_va) {
  if(fault_va >= MAXVA)
    return -1;
  pte_t* pte = walk(pagetable, fault_va, 0);          //查找虚拟地址 va 对应的页表条目（PTE）

  if(pte == 0)
    return -1;
  if((*pte & PTE_V) == 0)
    return -1;
  return (*pte & PTE_F ? 0 : -1);

  // if(*pte & PTE_F){
  //   return 0;
  // } else{
  //   return -1;
  // }
}

void*
cowalloc(pagetable_t pagetable, uint64 fault_va) {
  if(fault_va % PGSIZE != 0)
    return 0;

  fault_va = PGROUNDDOWN(fault_va);
  uint64 pa = walkaddr(pagetable, fault_va);  // 获取对应的物理地址
  if(pa == 0)
    return 0;

  pte_t* pte = walk(pagetable, fault_va, 0);  // 获取对应的页表项PTE

  if(ref.cnt[(uint64)pa / PGSIZE] == 1) {     // 只剩一个进程对此物理地址存在引用，没有其他进程共享该页面，允许写
    *pte |= PTE_W;
    *pte &= ~PTE_F;
    return (void*)pa;
  } else {
    char *mem;//分配内存页
    if((mem = kalloc()) == 0)  
      return 0; 
    memmove(mem, (char*)pa, PGSIZE);     //将物理地址 pa 处的数据复制到新分配的内存 mem 中

    *pte &= ~PTE_V;           //暂时清除PTE_V，防止其他进程访问，保证数据一致性
    
    if(mappages(pagetable, fault_va, PGSIZE, (uint64)mem, (PTE_FLAGS(*pte) | PTE_W) & ~PTE_F) != 0){ //将新分配的内存 mem 映射到新页表中
      kfree(mem);  //失败，释放之前分配的内存
      *pte |= PTE_V;
      return 0;
    }
    // 将原来的物理内存引用计数减1
    kfree((char*)PGROUNDDOWN(pa));
    return mem;
  }
}

// 增加内存的引用计数
int kaddrefcnt(void* pa) {
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&ref.lock);
  ++ref.cnt[(uint64)pa / PGSIZE];
  release(&ref.lock);
  return 0;
}