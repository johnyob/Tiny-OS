//
// Created by Alistair O'Brien on 6/20/2020.
//

#ifndef TINY_OS_DEBUG_H
#define TINY_OS_DEBUG_H

#include <lib/stdio.h>

void kernel_panic(const char* file, int line, const char* function, const char* message, ...);

#define panic(...) kernel_panic(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define assert(x)                               \
    if (!(x)) {                                 \
        panic("assertion '%s' failed.", #x);    \
    }

#define info(format, ...) printf("\x1b[0;36m[INFO]\x1b[0m " format, __VA_ARGS__)
#define warn(format, ...) printf("\x1b[0;33m[WARN]\x1b[0m " format, __VA_ARGS__)

#define NOT_REACHABLE panic("executed an unreachable statement")

#endif //TINY_OS_DEBUG_H
