//
// Created by Alistair O'Brien on 10/5/2020.
//

#include <lib/list.h>
#include <lib/stdbool.h>
#include <lib/string.h>

#include <debug.h>

#include <threads/synch.h>

#include <mm/pmm.h>
#include <mm/malloc.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DYNAMIC MEMORY ALLOCATION (MALLOC)                                                                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tiny OS provides a naive implementation of malloc, which is utilized through the kernel and in user programs.
//
// Let us first define a block as a fixed size of managed physical memory that lies on the kernel heap. In
// our naive implementation, we store blocks whose sizes are powers of two. Hence when requesting a new block
// of size n, in bytes, we round n up to a power of 2.
//
// Blocks are managed using block desciptors, which describe blocks of size n. The blocks are managed using a
// free list. If the free list is non-empty, then we simply pop a block off the free list and return it.
// Otherwise, we allocate a superblock (obtained from the page allocator). The superblock is then divided into
// blocks, all of which are added to the block descriptor's free list. We then simply pop one of the
// newly allocated blocks from the free list and return it.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct {
    list_t free_list;
    lock_t lock;

    size_t block_size;
} bucket_t;

#define SBLOCK_MAGIC            (0x9a548eed)
typedef enum {
    SBLOCK_MULTIBLOCK,
    SBLOCK_UNIBLOCK
} sblock_type_t;

typedef struct {
    uint64_t magic;

    sblock_type_t type;
    union {
        struct {
            bucket_t* bucket;
            size_t free_blocks;
        };
        struct {
            size_t page_order;
        };
    };
} sblock_t;

typedef struct {
    list_node_t list_node;
} block_t;


#define MIN_BLOCK_ORDER             (4)
#define MAX_BLOCK_ORDER             (PAGE_SHIFT - 1)
#define NUM_BUCKET                  (MAX_BLOCK_ORDER - MIN_BLOCK_ORDER)

static bucket_t buckets[NUM_BUCKET];


void malloc_init(void) {
    for (size_t i = 0; i < NUM_BUCKET; i++) {
        bucket_t* b = &buckets[i];

        list_init(&b->free_list);
        lock_init(&b->lock);

        b->block_size = 1 << (i + MIN_BLOCK_ORDER);
    }
}

static inline bool is_sblock(sblock_t* sb) {
    return sb != null && sb->magic == SBLOCK_MAGIC;
}

static size_t blocks_per_sblock(sblock_t* sb) {
    assert(is_sblock(sb));

    switch (sb->type) {
        case SBLOCK_UNIBLOCK:
            return 1;
        case SBLOCK_MULTIBLOCK:
            return (PAGE_SIZE - sizeof(sblock_t)) / (sb->bucket->block_size);
        default:
            panic("Unknown sblock type: %d.", sb->type);
            break;
    }

    NOT_REACHABLE;
}

static sblock_t* block_to_sblock(block_t* b) {
    sblock_t* sb = (sblock_t*)PAGE_ROUND_DOWN((uintptr_t)b);
    assert(is_sblock(sb));

    // In the case that we have a sblock with multiple blocks, ensure the block is aligned to the calculated block_size.
    // In the case we have a sblock that represents a single block, ensure the block starts after the superblock descriptor.
    assert((sb->type == SBLOCK_MULTIBLOCK
            && (PAGE_OFFSET((uintptr_t)b) - sizeof(sblock_t)) % sb->bucket->block_size == 0)
        || (sb->type == SBLOCK_UNIBLOCK
            && PAGE_OFFSET((uintptr_t)b) == sizeof(sblock_t))
    );

    return sb;
}

static size_t sblock_to_block_size(sblock_t* sb) {
    assert(is_sblock(sb));

    switch (sb->type) {
        case SBLOCK_UNIBLOCK:
            return (1 << (sb->page_order + PAGE_SHIFT)) - sizeof(sblock_t);
        case SBLOCK_MULTIBLOCK:
            return sb->bucket->block_size;
        default:
            panic("Unknown sblock type: %d.", sb->type);
            break;
    }

    NOT_REACHABLE;
}

static block_t* sblock_to_block(sblock_t* sb, size_t i) {
    assert(is_sblock(sb));

    assert(i < blocks_per_sblock(sb));

    size_t block_size = sblock_to_block_size(sb);

    return (block_t*)(
        (uintptr_t)sb
        + sizeof(sblock_t)
        + i * block_size);
}

static inline size_t block_size(block_t* block) {
    sblock_t* sb = block_to_sblock(block);
    return sblock_to_block_size(sb);
}

void* malloc(size_t size) {
    if (size == 0) return null;

    // Let us determine the minimum order [i] that satisfies the request
    size_t i;
    for (i = 0; i < NUM_BUCKET && size >= buckets[i].block_size; i++);

    // If the order [i] is greater than the maximum order [NUM_BUCKET - 1], then it follows
    // that we need to allocate a superblock.
    if (i == NUM_BUCKET) {
        // The order [i] is too big for any bucket, so we must allocate a uni superblock.
        // A superblock is a page aligned block of contiguous memory that may be split into smaller
        // blocks.

        // We first need to allocate enough pages for [size] bytes and the superblock descriptor.
        // So update [size] to include the size of the superblock descriptor. Allocate the pages.
        size_t sb_page_order = page_order(CEIL(size + sizeof(sblock_t), PAGE_SIZE));

        // If null is returned then there are no available pages of the requested size.
        // So return null.
        sblock_t* sb = alloc_pages(sb_page_order);
        if (sb == null) return null;

        // We now set the superblock metadata. Note that in this context, a block = page, hence the
        // number of free blocks = number of allocated pages.
        sb->magic = SBLOCK_MAGIC;
        sb->type = SBLOCK_UNIBLOCK;
        sb->page_order = sb_page_order;

        // Return the pointer to the free space (just after the superblock descriptor).
        return sb + 1;
    }

    bucket_t* bucket = &buckets[i];
    lock_acquire(&bucket->lock);

    // If our bucket is empty, then it follows that we need to allocate a new superblock and split it into blocks
    // of order [i]
    if (list_size(&bucket->free_list) == 0) {
        sblock_t* sb = alloc_pages(0);
        if (sb == null) {
            lock_release(&bucket->lock);
            return null;
        }

        // Set superblock metadata.
        sb->magic = SBLOCK_MAGIC;
        sb->type = SBLOCK_MULTIBLOCK;
        sb->bucket = bucket;

        // The number of free blocks of order [i] is given by size / 2^i where size is the available free space
        // So it follows that size = PAGE_SIZE - sizeof(superblock descriptor).
        sb->free_blocks = (PAGE_SIZE - sizeof(sblock_t)) / bucket->block_size;

        for (size_t j = 0; j < sb->free_blocks; j++) {
            block_t* block = sblock_to_block(sb, j);
            list_push_tail(&bucket->free_list, &block->list_node);
        }

    }

    block_t* block = LIST_VALUE(block_t, list_node, list_pop_head(&bucket->free_list));

    sblock_t* sb = block_to_sblock(block);
    assert(sb->type == SBLOCK_MULTIBLOCK);
    sb->free_blocks--;

    lock_release(&bucket->lock);

    return block;
}

void* calloc(size_t num, size_t size) {
    size_t s = num * size;

    void* p = malloc(s);
    if (p != null) bzero(p, s);

    return p;
}

void* realloc(void* old_block, size_t new_size) {
    if (new_size == 0) { free(old_block); return null; }

    void* new_block = malloc(new_size);
    if (old_block != null && new_block != null) {
        size_t old_size = block_size(old_block);
        memcpy(new_block, old_block, MIN(old_size, new_size));
        free(old_block);
    }

    return new_block;
}

void free(void* ptr) {
    if (ptr == null) return;

    block_t* block = (block_t*)ptr;
    sblock_t* sb = block_to_sblock(block);

    switch (sb->type) {
        case SBLOCK_UNIBLOCK:
            free_pages(sb, sb->page_order);
            break;
        case SBLOCK_MULTIBLOCK: {
            bucket_t* bucket = sb->bucket;
            bzero(block, bucket->block_size);

            lock_acquire(&bucket->lock);

            list_push_head(&bucket->free_list, &block->list_node);

            size_t bpsb = blocks_per_sblock(sb);
            if (++sb->free_blocks >= bpsb) {
                assert(sb->free_blocks == bpsb);

                for (size_t i = 0; i < bpsb; i++) {
                    block_t* b = sblock_to_block(sb, i);
                    list_delete(&bucket->free_list, &b->list_node);
                }

                free_page(sb);
            }

            lock_release(&bucket->lock);
            break;
        }
        default:
            panic("Unknown sblock type: %d.", sb->type);
            break;
    }
}