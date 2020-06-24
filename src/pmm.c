////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/21/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        mm.c
//      Environment: Tiny OS
//      Description: Contains page_alloc() and page_free(void* ptr) methods
//                   using a bookkeeping list and a free list.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>
#include <lib/stdbool.h>
#include <lib/string.h>
#include <lib/list.h>

#include <debug.h>
#include <symbols.h>
#include <pmm.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PAGE ALLOCATOR (PHYSICAL MEMORY MANAGER)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocates memory in multiples of page-size chunks.
//
// There are typically 2 ways to implement a page allocator:
//  1.  A free list. A free list is a singly linked list containing
//      a list of free pages.
//  2.  A page list. Sometimes referred to as a bookkeeping list.
//      A singly linked list of pages with struct fields
//      indicating attributes about the page e.g.  length, allocated, kernel, user, etc
//  3.  A bitmap. Instead of a list we store a collection of bitmaps. The bit 1 at position
//      i => page i is allocated, otherwise i is free.
//
// Our implementation uses a page list [pages] with a free list [free_list] for O(1)
// alloc_page and free_page.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    bool allocated;
} flags_t;

typedef struct page {
    flags_t flags;
    DEFINE_LINK(page)
} page_t;

// Implement and define singly linked list using macros
DEFINE_LIST(page)
IMPLEMENT_LIST(page)

// Static array containing all pages (allocated and free)
static page_t* pages;
static size_t length;

page_list_t free_list;


/*
 * Procedure:   init_pmm
 * ---------------------
 * Initializes the physical memory manager. Due to the use of paging in RISC-V, our
 * physical memory manager manages memory using pages, PAGE_SIZE (4096) byte blocks
 * of physical memory. This consists of allocating space for the bookkeeping list
 * [pages]. The free list requires no additional space on top of the bookkeeping list
 * since each element in [pages] has the next and prev pointers required for the free
 * list. After initializing the bookkeeping list and free list, we allocate the necessary
 * pages to the pmm (the pages required to store the bookkeeping list).
 *
 */
void init_pmm() {

    // The head of the array is at HEAP_START with length of HEAP_SIZE / PAGE_SIZE
    pages = (page_t*)(HEAP_START);
    length = HEAP_SIZE / PAGE_SIZE;

    // Initialize the array by zeroing out all fields
    size_t pages_size = sizeof(page_t) * length;
    bzero(pages, pages_size);

    // Initialize the free list which cleverly requires no additional space :)
    INIT_LIST(free_list);

    // We now need to allocate the pages used by the allocator to store this page metadata.
    // Note that we require pages_size = sizeof(page_t) * length bytes to store all the page metadata
    // Hence we require CEIL(page_size, PAGE_SIZE) pages.

    uint32_t i, pmm_pages = CEIL(pages_size, PAGE_SIZE);
    for (i = 0; i < pmm_pages; i++) {
        pages[i].flags.allocated = true;
    }

    // Map the rest of the pages as unallocated, and add them to the free list
    for(; i < length; i++) {
        pages[i].flags.allocated = false;
        insert_page_list(&free_list, &pages[i]);
    }
}


/*
 * Function:   alloc_page
 * ----------------------
 * The alloc_page function returns the pointer of a free page.
 * The page is taken from the free list, it's flags are updated
 * and it's respective page frame is zeroed.
 *
 * @returns: A pointer to the newly allocated free page.
 *           Returns a null pointer if there are no available free
 *           pages.
 *
 */
void* alloc_page() {
    if (size_page_list(&free_list) == 0) {
        return null;
    }

    // Get a free page
    page_t* p = peek_page_list(&free_list);
    delete_page_list(&free_list, p);

    // Set flags
    p->flags.allocated = true;

    // Determine pointer of the start of the page
    // Zero out the page
    void* ptr = (void*)((p - pages) * PAGE_SIZE);
    bzero(ptr, PAGE_SIZE);

    return ptr;
}


/*
 * Procedure:   free_page
 * ----------------------
 * The free_page procedure frees an allocated page.
 *
 * @void* ptr:  The physical memory address of the page to be freed.
 *              The address must be page alligned and is a member
 *              of the interval [HEAP_START, MEMORY_END).
 *
 */
void free_page(void* ptr) {

    assert((uintptr_t)ptr % PAGE_SIZE == 0);
    assert((uintptr_t)ptr >= HEAP_START && (uintptr_t)ptr < MEMORY_END);

    // Get the page metadata from the ptr
    page_t* p = pages + ((uintptr_t)ptr / PAGE_SIZE);

    // Assert that the page has been allocated. We shouldn't be freeing allocated pages...
    assert(p->flags.allocated);

    // Zero out the page
    bzero(ptr, PAGE_SIZE);

    // Set flags
    p->flags.allocated = false;

    // Add to the free list
    insert_page_list(&free_list, p);
}
