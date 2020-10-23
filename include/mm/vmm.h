//
// Created by Alistair O'Brien on 6/24/2020.
//

#ifndef TINY_OS_VMM_H
#define TINY_OS_VMM_H

#include <lib/stdint.h>

// Page Table Entry (PTE) Macros

typedef uint64_t    pte_t;
typedef uint64_t    paddr_t;
typedef uint64_t    vaddr_t;

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

#define PTE_MODE_MASK (0xe) // Read, write and execute bits

#define PTE_SHIFT 10
#define PTE_FLAGS_MASK 0x3ff // 10 bits
#define PTE_FLAGS(pte) ((pte) & PTE_FLAG_MASK)

#define PTE_PPN0(pte) (((pte_t)pte >> 10) & 0x1ff)
#define PTE_PNN1(pte) (((pte_t)pte >> 19) & 0x1ff)
#define PTE_PNN2(pte) (((pte_t)pte >> 28) & 0x3ffffff)

#define PTE_TO_PADDR(pte) (((((paddr_t)pte) >> PTE_SHIFT) & PPN_MASK) << OFFSET_SHIFT)

// Virtual Address Macros

// extract the 9-bit virtual page numbers (VPN)
#define OFFSET_SHIFT 12
#define OFFSET_MASK (0xfff)

#define VPN_MASK 0x1ff // 9 bits

#define VPN_SHIFT(level) (OFFSET_SHIFT + (9 * (level)))
#define VPN(vaddr, level) ((((vaddr_t)vaddr) >> VPN_SHIFT(level)) & VPN_MASK)

// Physical Address Macros

// define a physical address as PNN2 | PPN1 | PPN0 | page offset
#define PPN_MASK (0xfffffffffff)

#define PADDR_PNN0(paddr) ((((paddr_t)paddr) >> 12) & 0x1ff)
#define PADDR_PNN1(paddr) ((((paddr_t)paddr) >> 21) & 0x1ff)
#define PADDR_PPN2(paddr) ((((paddr_t)paddr) >> 30) & 0x3ffffff)

#define PADDR_TO_PTE(paddr) (((((paddr_t)paddr) >> OFFSET_SHIFT) & PPN_MASK) << PTE_SHIFT)

// SATP Sv39
#define SATP_SV39 (8L << 60)
#define SATP(table) (SATP_SV39 | (((uint64_t)table) >> 12))

// Maximum virtual address
// To avoid sign-extended virtual address we remove the top bit
// Hence 38 bits instead of 39.
#define MAX_VADDR (1L << 38)

// 512 entries (essentially pte_t[512])
typedef pte_t* pagetable_t;

void vmm_init();
void vmm_hart_init();

paddr_t walk(pagetable_t table, vaddr_t vaddr);
void map(pagetable_t table, vaddr_t vaddr, paddr_t paddr, size_t length, uint32_t perm);
void unmap(pagetable_t table, vaddr_t vaddr, size_t length);

pagetable_t kpagetable(void);
void kmap(vaddr_t vaddr, paddr_t paddr, size_t length, uint32_t perm);
void kunmap(vaddr_t vaddr, size_t length);


#endif //TINY_OS_VMM_H
