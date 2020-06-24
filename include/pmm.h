//
// Created by Alistair O'Brien on 6/22/2020.
//

#ifndef TINY_OS_MM_H
#define TINY_OS_MM_H

#include <lib/stddef.h>
#include <lib/stdint.h>

#define PAGE_SIZE 4096
#define PAGE_ROUND_DOWN(p)  (ROUND_DOWN(p, PAGE_SIZE))
#define PAGE_ROUND_UP(p)    (ROUND_UP(p, PAGE_SIZE))

void init_pmm();
void* alloc_page();
void free_page(void* ptr);

#endif //TINY_OS_MM_H
