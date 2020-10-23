////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/21/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        pmm.c
//      Environment: Tiny OS
//      Description: Contains alloc_pages(size_t order) and free_pages(void* ptr, size_t order) methods
//                   using a binary buddy allocator and a bitmap.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>
#include <lib/stdbool.h>
#include <lib/string.h>
#include <lib/list.h>

#include <debug.h>

#include <mm/symbols.h>
#include <mm/pmm.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PAGE ALLOCATOR (PHYSICAL MEMORY MANAGER)                                                                           //
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
// We implement a binary buddy memory allocator which allocates free memory within a fixed *linear* address space.
// We keep track of free pages using structures called "blocks", a block of order n
// refers to a free collection of 2^n pages.
//
// The "buddy" term comes from how a tree is used. When memory is allocated,
// blocks are recursively (or iteratively) split until a block of appropriate size is reached.
// When a block is split, it produces two blocks, the smaller block and it's buddy (sibling).
//
// If the block is freed later, we can merge the block with it's buddy (if its also free). We keep track of whether
// a buddy is free using a bitmap.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BITMAP METHODS                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * We define a bitmap struct (or bitmap_t) as a map; an array of bitmaps stored in a list of undetermined size
 * with initial pointer [map].
 * For memory management reasons, we also store the size of the map list (in bytes) using the field [size].
 */
typedef struct {
    uint64_t*   map;
    size_t      size;
} bitmap_t;

// mm_bitmap is the memory manager's global bitmap, since we only need a single bitmap.
static bitmap_t mm_bitmap;

/*
 * Due to our implementation of a bitmap_t using a uint64_t map array, the number of
 * page bits (a page bit in a bitmap indiciates whether the page is allocator or not)
 * per word (or element in the [map] array) is given by the number of bits of a single
 * element in the [map] array.
 */
#define PAGES_PER_WORD          (sizeof(uint64_t) * 8)

/*
 * To access a page bit in the [map] array, we must consider the index of the page
 * within the [map] array and the offset of the page bit within the particular element
 * stored in the [map] array.
 */
#define PAGE_NUM_TO_INDEX(p)    (p / PAGES_PER_WORD)
#define PAGE_NUM_TO_OFFSET(p)   (p & (PAGES_PER_WORD - 1))

/*
 * Updating the [map] array requires some bitwise-magic.
 * The macro BITMAP_MASK_GEQ(n) produces the bitstring: 11 ... 1100000
 * consisting of 1s for all bits >= n.
 * Similarly, the macro BITMAP_MASK_LT(n) produces the bitstring: 00 ... 00111111
 * consisting of 1s for all bits < n.
 */
#define BITMAP_MASK_GEQ(n)      (-(1 << n))
#define BITMAP_MASK_LT(n)       ((1 << n) - 1)

/*
 * To use [map], we need a quick way to test whether a given page has been allocated.
 * We can simply use the PAGE_NUM_TO_INDEX and PAGE_NUM_TO_OFFSET macros
 * we defined earlier, since a page p is allocated <=> the PAGE_NUM_TO_OFFSET(p) bit is set in
 * the PAGE_NUM_TO_INDEX(p) element of mm_bitmap.map.
 */
#define ALLOCATED(p)            (mm_bitmap.map[PAGE_NUM_TO_INDEX(p)] & (1 << PAGE_NUM_TO_OFFSET(p)))

/*
 * Procedure:   bitmap_alloc
 * -------------------------
 * The bitmap_alloc(p, size) procedure allocates the pages in the range [p, p + size), thus allocating
 * [size] contiguous pages in the bitmap mm_bitmap. We proceed by computing the start and end indexes and
 * offsets. We then have two cases:
 *  1. start_idx = end_idx =>
 *      The page bits that need to be set are in mm_bitmap.map[start_idx].
 *      So we need to set all bits >= start_off and set all bits < end_off.
 *
 *  2. start_idx != end_idx =>
 *      The page bits to be set span across multiple bitmap words.
 *      So we need to set all bits >= start_off in mm_bitmap.map[start_idx].
 *      Set all bits between start_idx and end_idx and then
 *      Set all bits < end_off in mm_bitmap.map[end_idx].
 *
 * Note: We assume that the pages in the range [p, p + size) are already free.
 *
 * @uint64_t p:     The page frame number of the page at the start of the contiguous block of
 *                  [size] pages.
 *
 * @size_t size:    The number of pages to be allocated.
 *
 */
void bitmap_alloc(uint64_t p, size_t size) {
    uint64_t start_idx, start_off, end_idx, end_off;

    start_idx = PAGE_NUM_TO_INDEX(p);
    start_off = PAGE_NUM_TO_OFFSET(p);
    end_idx = PAGE_NUM_TO_INDEX(p + size);
    end_off = PAGE_NUM_TO_OFFSET(p + size);

    if (start_idx == end_idx) {
        // Case 1. Set all bits in range [start_off, end_off). The bitstring with these bits set
        // is BITMAP_MASK_GEQ(start_off) & BITMAP_MASK_LT(end_off).

        mm_bitmap.map[start_idx] |= BITMAP_MASK_GEQ(start_off) & BITMAP_MASK_LT(end_off);
    } else {
        // Case 2. Set all bits >= start_off in mm_bitmap.map[start_idx].
        mm_bitmap.map[start_idx] |= BITMAP_MASK_GEQ(start_off);

        // Then set all bits between start_idx and end_idx.
        while (++start_idx < end_idx) mm_bitmap.map[start_idx] = ~0L;

        // Finally set the bits < end_off in mm_bitmap.map[end_idx].
        mm_bitmap.map[start_idx] |= BITMAP_MASK_LT(end_off);
    }
}

/*
 * Procedure:   bitmap_free
 * -------------------------
 * The bitmap_free(p, size) procedure frees the pages in the range [p, p + size), thus freeing
 * [size] contiguous pages in the bitmap mm_bitmap. We proceed by simply performing the reverse
 * operations to that of [bitmap_alloc].
 *
 * Note: We assume that the pages in the range [p, p + size) are already allocated.
 *
 * @uint64_t p:     The page frame number of the page at the start of the contiguous block of
 *                  [size] pages.
 *
 * @size_t size:    The number of pages to be freed.
 *
 */
void bitmap_free(uint64_t p, size_t size) {
    uint64_t start_idx, start_off, end_idx, end_off;

    start_idx = PAGE_NUM_TO_INDEX(p);
    start_off = PAGE_NUM_TO_OFFSET(p);
    end_idx = PAGE_NUM_TO_INDEX(p + size);
    end_off = PAGE_NUM_TO_OFFSET(p + size);

    if (start_idx == end_idx) {
        mm_bitmap.map[start_idx] &= BITMAP_MASK_LT(start_off) | BITMAP_MASK_GEQ(end_off);
    } else {
        mm_bitmap.map[start_idx] &= BITMAP_MASK_LT(start_off);
        while (++start_idx < end_idx) mm_bitmap.map[start_idx] = 0L;
        mm_bitmap.map[start_idx] &= BITMAP_MASK_GEQ(end_off);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BINARY BUDDY ALLOCATOR                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * We define a block struct (or block_t) as consisting of a [order], since a block of order n consists of 2^n
 * free contiguous pages. We also define a linked list link between blocks. This will be used when managing *buckets*
 * of blocks.
 */

typedef struct block {
    size_t order;
    list_node_t list_node;
} block_t;


/*
 * We define a bucket of order n as a collection of blocks (that need not be contiguous) that are of the same order,
 * namely of order n.
 * We will have 9 bucket; buckets of order: 0, 1, 2, 3, 4, 5, 6, 7, 8.
 */
#define BUCKET_COUNT 9

// The bucket of order i is stored in the buckets list at index i, that is buckets[i].
static list_t buckets[BUCKET_COUNT];

// Our memory manager needs to keep track of the minimum (base) pointer of the heap
// and the maximum pointer.
static uintptr_t base_ptr;
static uintptr_t max_ptr;

/*
 * Our allocator requires us to often quickly switch between page frame numbers and page frame addresses.
 * To do so, we use some simple macros: PAGE_NUM_TO_ADDR(p) converts the page frame number p to it's
 * respective physical address.
 * Similarly, the ADDR_TO_PAGE_NUM(ptr) macro converts the page frame address (given by the ptr) to
 * it's respective page frame number p.
 *
 * Note: PAGE_NUM_TO_ADDR(ADDR_TO_PAGE_NUM(ptr)) = ptr (and vice versa).
 */
#define PAGE_NUM_TO_ADDR(p)         (base_ptr + (p << PAGE_SHIFT))
#define ADDR_TO_PAGE_NUM(ptr)       ((ptr - base_ptr) >> PAGE_SHIFT)

/*
 * Function:    alloc_pages
 * ------------------------
 * The alloc_pages function returns the pointer to a block of 2^[order] free contiguous pages.
 * The page is taken from the binary buddy allocator tree using the binary buddy allocator algorithm:
 *  1. Find the order i >= [order] s.t the bucket of order i isn't empty.
 *  2. Remove a block from the bucket of order i.
 *  3. Recursively (or iteratively, since it's a tail-recursive algorithm)
 *     split the block until the split-block has order = [order].
 *  4. Return the point to the split-block.
 *
 * The algorithm has a time complexity of O(log N) where log N is the number of buckets (N is the number of page frames).
 * (Our implementation is limited to 9, however, we could have a single block representing the entire heap).
 *
 * @size_t order:   The order of the block to be allocated, that is to say it specifies the number of pages to be
 *                  allocated, specifically 2^[order] pages.
 *
 * @returns:        A pointer to the newly allocated free page.
 *                  Returns a null pointer if there are no available free
 *                  pages.
 *
 */
void* alloc_pages(size_t order) {

    // Increment the order [i] until we either reach the end of the bucket array,
    // or we find a bucket that isn't empty.
    size_t i;
    for (i = order; i < BUCKET_COUNT && list_size(&buckets[i]) == 0; i++);

    // If [i == BUCKET_COUNT], then it follows that we didn't find a non-empty bucket.
    // Hence we have no available contiguous blocks for the request, so return null.
    if (i == BUCKET_COUNT) return null;

    // Since [i != BUCKET_COUNT], then it follows that we found a non-emtpy bucket :)
    // So remove a block from the bucket of order [i].
    block_t* free_block = LIST_VALUE(block_t, list_node, list_pop_head(&buckets[i]));

    // We break the block into smaller blocks until we have a block of order [order].
    block_t* buddy_block;
    while (i != order) {
        // Decrement the current order [i].
        i--;

        // The right buddy block of order [i - 1] is [2^(i - 1) * PAGE_SIZE = 1 << ((i - 1) + PAGE_SHIFT)] bytes from
        // the original block.
        buddy_block = (block_t*)((uintptr_t)free_block + (1 << (i + PAGE_SHIFT)));

        // Insert the free buddy block into the bucket of order [i - 1].
        list_push_head(&buckets[i], &buddy_block->list_node);
    }

    // Allocate the pages within the free_block of order [i = order].
    bitmap_alloc(ADDR_TO_PAGE_NUM((uintptr_t)free_block), 1 << order);

    // Zero the contents of the block.
    bzero(free_block, 1 << (order + PAGE_SHIFT));

    return free_block;
}

/*
 * Procedure:   free_pages
 * -----------------------
 * The free_pages procedure frees a block of 2^[order] allocated contiguous pages.
 * The algorithm first frees the block from the bitmap.
 * We then recursively (or iteratively, since the algorithm is tail recursive) merge buddy blocks together until we have a
 * block of order 8 (the maximum order) or reach a buddy that is of different order => the buddy's buddy (or it's child)
 * is allocated. Or we find an allocated buddy.
 *
 * The algorithm has a time complexity of O(log N) where log N is the number of buckets (N is the number of page frames).
 * (Our implementation is limited to 9, however, we could have a single block representing the entire heap).
 *
 * @void* ptr:      The pointer to the beginning of the allocated block that is to be freed.
 *
 * @size_t order:   The order of the block to be freed, that is to say it specifies the number of pages to be
 *                  freed, specifically 2^[order] pages.
 *
 */
void free_pages(void* ptr, size_t order) {

    block_t* freed_block = (block_t*)ptr;
    block_t* buddy_block;

    // Free the block from the bitmap
    uint64_t p = ADDR_TO_PAGE_NUM((uintptr_t)freed_block);
    bitmap_free(p, 1 << order);

    while (order < BUCKET_COUNT) {

        // We need to find [freed_block]'s buddy, we do so by performing some pointer magic.
        // We note that a block is a right child iff exists n s.t p = (2n + 1) * 2^order = 2k + 2^order,
        // where k = 2^{order + 1} * n. Hence we may determine whether the freed_block is a
        // right child if the order'th bit is set.

        uint64_t mask = 1 << order;
        buddy_block = (block_t*)((uintptr_t)freed_block + (p & mask ? -mask : mask));

        // If the buddy is allocated, or the buddy has a different order => the buddy's buddy (or its child) is allocated.
        // Then we cannot coalesce. So break.
        if (ALLOCATED(ADDR_TO_PAGE_NUM((uintptr_t)buddy_block)) || buddy_block->order != order) break;

        // If the buddy is free, then remove it from the bucket of order [order].
        list_delete(&buckets[order], &buddy_block->list_node);

        // If freed_block is a right child, then freed_block pointer needs to be updated to the start of the new block
        // which is the pointer to the buddy_block. We also need to update the page number of the freed_block.
        if (p & mask) {
            freed_block = buddy_block;
            p = ADDR_TO_PAGE_NUM((uintptr_t)freed_block);
        }

        // Increment the order since freed_block is now of size 2 * 2^order = 2^{order + 1} pages.
        order++;
    }

    // Set the new order of the freed_block.
    freed_block->order = order;

    // Insert the free block into the bucket of order [order].
    list_push_head(&buckets[order], &freed_block->list_node);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PHYSICAL MEMORY MANAGER INITIALIZATION                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Procedure:   pmm_init
 * ---------------------
 * Initializes the physical memory manager. Due to the use of paging in RISC-V, our
 * physical memory manager manages memory using pages, PAGE_SIZE (4096) byte blocks
 * of physical memory.
 * pmm_init first allocates the space for the bitmap [mm_bitmap].
 * The buddy block structures (and bucket lists) do not require any additional space
 * since the buckets are stored in BSS and the blocks are stored in their respective
 * free blocks. After initializing the bitmap, we initialize the buckets and fill
 * them with the available blocks.
 *
 */
void pmm_init() {

    // Determine the number of bytes required by the bitmap.
    // We note that size = (HEAP_SIZE / PAGE_SIZE) / 8 = HEAP_SIZE >> (PAGE_SHIFT + 3).
    mm_bitmap.size = HEAP_SIZE >> (PAGE_SHIFT + 3);

    // The bitmap is allocated the space at the start of the heap.
    mm_bitmap.map = (uint64_t*)(HEAP_START);

    // The base pointer of the memory manager is set after the bitmap.
    // It must be page aligned.
    base_ptr = PAGE_ROUND_UP(HEAP_START + mm_bitmap.size);

    // The max_ptr of the memory manager is MEMORY_END (which is page aligned by definition).
    max_ptr = MEMORY_END;

    // We calculate the range for determining number of pages and
    // initializing the bucket list.
    uint64_t range = (max_ptr - base_ptr) >> PAGE_SHIFT;

    // We allocate all pages by default (avoids attempting to allocate pages that are "free" in the bitmap, but
    // actually allocated).
    memset(mm_bitmap.map, ~0, mm_bitmap.size);

    // We now free up the memory we *know* is free.
    bitmap_free(ADDR_TO_PAGE_NUM(base_ptr), range);

    // Initialize the buckets
    for (int i = 0; i < BUCKET_COUNT; i++) list_init(&buckets[i]);

    block_t* block;
    uintptr_t min = base_ptr;
    int i;

    while (range > 0) {
        // Find the maximum order i >= 0 s.t i < BUCKET_COUNT and 2^i <= range
        for (i = BUCKET_COUNT - 1; i >= 0 && (1 << i) > range; i--);

        // If i = -1 => for all i in [0, BUCKET_COUNT). 2^i > range. Hence range = 0.
        // Since range > 0 => this case cannot occur.

        // Create block and insert it into the correct bucket.
        block = (block_t*)min;
        block->order = i;
        list_push_head(&buckets[i], &block->list_node);

        // Modify min and range accordingly with the allocated block.
        min += 1 << (i + PAGE_SHIFT);
        range -= 1 << i;
    }
}
