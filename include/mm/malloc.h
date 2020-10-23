//
// Created by Alistair O'Brien on 10/5/2020.
//

#ifndef TINY_OS_MALLOC_H
#define TINY_OS_MALLOC_H

#include <lib/stddef.h>

void malloc_init(void);

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);

#endif //TINY_OS_MALLOC_H
