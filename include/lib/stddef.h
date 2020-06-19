////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        stddef.h
//      Environment: Tiny OS
//      Description: C99 standard definition of null, size and ptrdiff.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_STDDEF_H
#define TINY_OS_STDDEF_H

#include <lib/stdint.h>

#define null ((void*)0)

typedef uint64_t ptrdiff_t;
typedef uint64_t size_t;

#endif //TINY_OS_STDDEF_H
