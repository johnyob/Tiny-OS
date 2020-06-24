////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/21/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        debug.c
//      Environment: Tiny OS
//      Description: debug implements the kernel_panic method which prints the location of the kernel panic
//                   and then prints what caused the kernel panic. We then enter an infinite loop.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <lib/stdbool.h>
#include <lib/stdarg.h>
#include <lib/stdio.h>

#include <debug.h>

/*
 * Procedure:   kernel_panic
 * -------------------------
 * This procedure, once invoked causes the kernel to panic, which consists of printing
 * some important debugging information followed by entering an infinite loop.
 *
 * @char* file:     The filename where the panic has occurred. e.g. pmm.c
 * @int line:       The line of the file where the panic occurred.
 * @char* function: The function where the panic occurred. e.g. alloc_page
 * @char* message:  A message indicating what caused the panic e.g. assertion blah failed.
 * @...:            Additional formatting arguments for the debugging message [message].
 *
 */
void kernel_panic(const char* file, int line, const char* function, const char* message, ...) {
    va_list va;
    va_start(va, message);

    printf("\x1b[0;31m[PANIC]\x1b[0m KERNEL PANIC at %s:%d in %s(): ", file, line, function);
    vprintf(message, va);
    printf("\n");

    va_end(va);

    while (true);
}