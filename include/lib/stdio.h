////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/20/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        stdio.h
//      Environment: Tiny OS
//      Description: C99 standard definition of printf methods, including printf, sprintf, snprintf,
//                   vprintf and vsnprintf. The header also contains the prototypes for the
//                   internal __printf and __vprintf methods. These are implemented by the console dev.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#ifndef TINY_OS_STDIO_H
#define TINY_OS_STDIO_H

#include <lib/stdarg.h>
#include <lib/stddef.h>

// Fast-fail support for unsecure methods
#define sprintf dont_use_sprintf_use_snprintf
#define vsprintf dont_use_vsprintf_use_vsnprintf

// Standard methods
int printf(const char* format, ...);
int snprintf(char* buffer, size_t n, const char* format, ...);

// The implementation of vprintf is provided in the console dev :)
int vprintf(const char* format, va_list va);
int vsnprintf(char* buffer, size_t n, const char* format, va_list va);

// Internal implementations (yikes, plz don't use unless you know what you're doing)
void __printf(const char* format, void (*putc)(char, void*), void* buf, ...);
void __vprintf(const char* format, va_list va, void (*putc)(char, void*), void* buf);



#endif //TINY_OS_STDIO_H
