////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        stdarg.h
//      Environment: Tiny OS
//      Description: C99 standard definition of variable arguments using GCC definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_STDARG_H
#define TINY_OS_STDARG_H

typedef __builtin_va_list va_list;

#define va_start(ap, last)              (__builtin_va_start(ap, last))
#define va_arg(ap, type)                (__builtin_va_arg(ap, type))
#define va_end(ap)                      (__builtin_va_end(ap))
#define va_copy(dst, src)               (__builtin_va_copy(dst, src))

#endif //TINY_OS_STDARG_H
