////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/24/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        vmm.c
//      Environment: Tiny OS
//      Description: The virtual memory manager (vmm) contains the methods for mapping virtual addresses to physical
//                   address. The vmm also initializes the kernel page table using an identity map.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/stdbool.h>

#include <riscv.h>
#include <debug.h>

#include <mm/symbols.h>
#include <mm/pmm.h>
#include <mm/vmm.h>


static pagetable_t pagetable;

/*
 * Procedure:   vmm_init
 * ---------------------
 *
 */
void vmm_init() {
    pagetable = (pagetable_t)alloc_page();

    // TEXT
    map(pagetable, TEXT_START, TEXT_START, TEXT_END - TEXT_START, PTE_R | PTE_X);
    info("text: \t%#p -> %#p\n", TEXT_START, TEXT_END);

    // RODATA
    map(pagetable, RODATA_START, RODATA_START, RODATA_END - RODATA_START, PTE_R);
    info("rodata: \t%#p -> %#p\n", RODATA_START, RODATA_END);

    // DATA
    map(pagetable, DATA_START, DATA_START, DATA_END - DATA_START, PTE_R | PTE_W);
    info("data: \t%#p -> %#p\n", DATA_START, DATA_END);

    // BSS
    map(pagetable, BSS_START, BSS_START, BSS_END - BSS_START, PTE_R | PTE_W);
    info("bss: \t%#p -> %#p\n", BSS_START, BSS_END);

    // Kernel Stack
    map(pagetable, STACK_START, STACK_START, STACK_END - STACK_START, PTE_R | PTE_W);
    info("stack: \t%#p -> %#p\n", STACK_START, STACK_END);

    // Heap
    map(pagetable, HEAP_START, HEAP_START, HEAP_SIZE, PTE_R | PTE_W);
    info("heap: \t%#p -> %#p\n", HEAP_START, MEMORY_END);

}

/*
 * Procedure:   vmm_hart_init
 * --------------------------
 */
void vmm_hart_init() {
    w_satp(SATP(pagetable));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL METHODS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Procedure:   __walk
 * -------------------
 * The internal walk procedure traverses the 3-level page table structure.
 */
pte_t* __walk(pagetable_t table, vaddr_t vaddr, bool can_alloc) {
    assert(vaddr < MAX_VADDR);

    // Walk the page tables.
    // We assert that a page table has 3 levels.
    for (int level = 2; level > 0; level--) {
        pte_t *pte = &table[VPN(vaddr, level)];

        // This is an invalid entry, allocate a new page or return null.
        if ((*pte & PTE_V) == 0) {
            // If we cannot allocate a new page, then return null.
            if (!can_alloc) return null;

            void* p = alloc_page();
            assert(p != null);
            *pte = PADDR_TO_PTE(p) | PTE_V;
        }

        table = (pagetable_t)PTE_TO_PADDR(*pte);
    }

    return &table[VPN(vaddr, 0)];
}

void map_page(pagetable_t table, vaddr_t vaddr, paddr_t paddr, uint32_t flags) {

    pte_t* pte = __walk(table, vaddr, true);

    // If the pte pointer = null, then something v wrong has occurred.
    assert(pte != null);
    *pte = PADDR_TO_PTE(paddr) | flags | PTE_V;
}

void unmap_page(pagetable_t table, vaddr_t vaddr) {

    pte_t* pte = __walk(table, vaddr, false);

    // If the pte pointer = null, then something v wrong has also occurred.
    assert(pte != null);

    *pte = 0;
    free_page((void*)PTE_TO_PADDR(*pte));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

paddr_t walk(pagetable_t table, vaddr_t vaddr) {

    pte_t* pte = __walk(table, vaddr, false);

    if (pte == null) return 0;
    return PTE_TO_PADDR(*pte) | (vaddr & OFFSET_MASK);
}


void map(pagetable_t table, vaddr_t vaddr, paddr_t paddr, size_t n, uint32_t perm) {
    vaddr_t vaddr_start = PAGE_ROUND_DOWN(vaddr);
    vaddr_t vaddr_end = PAGE_ROUND_DOWN(vaddr + n - 1); // vaddr of last page

    while (vaddr_start <= vaddr_end) {
        map_page(table, vaddr_start, paddr, perm);
        vaddr_start += PAGE_SIZE; paddr += PAGE_SIZE;
    }
}

void unmap(pagetable_t table, vaddr_t vaddr, size_t n) {
    vaddr_t vaddr_start = PAGE_ROUND_DOWN(vaddr);
    vaddr_t vaddr_end = PAGE_ROUND_DOWN(vaddr + n - 1); // vaddr of last page

    while (vaddr_start <= vaddr_end) {
        unmap_page(table, vaddr_start);
        vaddr_start += PAGE_SIZE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC KERNEL METHODS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline pagetable_t kpagetable(void) {
    return pagetable;
}

paddr_t kwalk(vaddr_t vaddr) {
    walk(pagetable, vaddr);
}

void kmap(vaddr_t vaddr, paddr_t paddr, size_t n, uint32_t perm) {
    map(pagetable, vaddr, paddr, n, perm);
}

void kunmap(vaddr_t vaddr, size_t n) {
    unmap(pagetable, vaddr, n);
}
