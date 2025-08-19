#include "pageswap.h"
#include "types.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "buf.h"
#include "mmu.h"
#include "proc.h"
struct swapslot swap_slots[NSLOT];

// Function to initialise swap slots - to be called after system booting
void 
initswap(void) {
    for (int i = 0; i < NSLOT; i++) {
        swap_slots[i].page_perm = 0;
        swap_slots[i].is_free = 1;
    }
    // cprintf("Swap: initialized %d swap slots (%d blocks)\n", 
        // NSLOT, NSWAP);
}

int
allocateswap(void)
{
  for(int i = 0; i < NSLOT; i++) {
    if(swap_slots[i].is_free) {
      swap_slots[i].is_free = 0;
      return i;
    }
  }
  return -1;  // No free slot available
}

// Error handling is pending
void
writeswap(int slot, char *page, int permissions)
{
  // cprintf("Writing to swap slot %d\n", slot);
  struct buf *bp;
  uint block = 2 + slot * NBPS;
  for(int i = 0; i < NBPS; i++) {
    bp = bread(ROOTDEV, block + i);  // Use bread to get a buffer
    memmove(bp->data, page + i * BSIZE, BSIZE);  // Copy 512 bytes
    bp->flags |= B_DIRTY;  // Mark as dirty
    iderw(bp);  // Write directly to disk, bypassing log
    brelse(bp);
  }
  swap_slots[slot].page_perm = permissions;
}

void 
readswap(int slot, char *page)
{
  struct buf *bp;
  char* dst = page;
  uint block = 2 + slot * NBPS;
  for(int i = 0; i < NBPS; i++) {
    bp = bread(ROOTDEV, block + i);  // Use bread to get a buffer
    memmove(dst, bp->data, BSIZE);  // Copy 512 bytes
    brelse(bp);
    dst += BSIZE;
  }
}

// make additional functions to clean / free swap slots when the process exits
void
freeswap(int slot)
{
  swap_slots[slot].is_free = 1;
  swap_slots[slot].page_perm = 0;
}

void 
cleanswap(struct proc* p) {
  pte_t *pte;
  uint va;

  for(va = PGROUNDDOWN(0); va < p->sz; va += PGSIZE) {
      pte = walkpgdir2(p->pgdir, (char*)va, 0);
      if (pte && !(*pte & PTE_P) && (*pte & PTE_U)) {
          int slot = PTE_ADDR(*pte) >> 12;
          if (slot >= 0 && slot < NSLOT) {
              freeswap(slot);
          }
          *pte = 0;  // Clear the page table entry
      }
  }
}