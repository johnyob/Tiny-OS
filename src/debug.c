//
// Created by Alistair O'Brien on 6/21/2020.
//
#include <lib/stdbool.h>
#include <lib/stdarg.h>
#include <lib/stdio.h>

#include <debug.h>

void kernel_panic(const char* file, int line, const char* function, const char* message, ...) {
    va_list va;
    va_start(va, message);

    printf("\x1b[0;31m[PANIC]\x1b[0m KERNEL PANIC at %s:%d in %s(): ", file, line, function);
    vprintf(message, va);
    printf("\n");

    va_end(va);

    while (true);
}