// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

int Threshold = 100;
int Npg = 4;

#define LIMIT 100

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  uint freepages;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.freepages++;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  if(kmem.freepages <= Threshold) {
    // cprintf("Free Pages = %d", kmem.freepages);
    cprintf("Current Threshold = %d, Swapping %d pages\n", Threshold, Npg);
    if(kmem.use_lock)
      release(&kmem.lock);
    for(int i = 0; i < Npg; i++) {
      // cprintf("Swapping %d page\n", i+1);
      struct proc* victimproc = findvictimproc();
      // cprintf("Victim Process = %d\n", victimproc->pid);
      if(!victimproc) {
        // cprintf("No victim process found\n");
        break;
      }
      pte_t *pte = getvictimpage(victimproc);
      if(!pte) {
        // cprintf("No victim page found\n");
        break;
      }

      uint pg_addr = PTE_ADDR(*pte);
      int pg_permission = PTE_FLAGS(*pte);
      int slot = allocateswap();
      if (slot == -1) break;
      writeswap(slot, P2V(pg_addr), (pg_permission & ~PTE_P));
      kfree(P2V(pg_addr));
      *pte = (slot << 12) | (pg_permission & ~PTE_P); 
      victimproc->rss--;
      // cprintf("Victim Process RSS = %d\n", victimproc->rss);
    }
    Threshold -= (Threshold*BETA) / 100;
    Npg += (Npg*ALPHA) / 100;
    if(Npg >= LIMIT) {
      Npg = LIMIT;
    }
    if(kmem.use_lock)
      acquire(&kmem.lock);
  }
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    kmem.freepages--;}
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

