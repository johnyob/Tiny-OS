//
// Created by Alistair O'Brien on 6/22/2020.
//

#ifndef TINY_OS_MM_H
#define TINY_OS_MM_H

#include <lib/stddef.h>
#include <lib/stdint.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE 4096
#define PAGE_ROUND_DOWN(p)  (ROUND_DOWN(p, PAGE_SIZE))
#define PAGE_ROUND_UP(p)    (ROUND_UP(p, PAGE_SIZE))

void pmm_init();

void* alloc_pages(size_t order);
void free_pages(void* ptr, size_t order);

// Define some useful macros.
#define alloc_page()    alloc_pages(0)
#define free_page(ptr)  free_pages(ptr, 0)

#endif //TINY_OS_MM_H
