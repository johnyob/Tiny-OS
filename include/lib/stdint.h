////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        stdint.h
//      Environment: Tiny OS
//      Description: C99 standard definition of variable sized signed and unsigned integer types.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_STDINT_H
#define TINY_OS_STDINT_H

typedef signed char         int8_t;
typedef signed short int    int16_t;
typedef signed int          int32_t;
typedef signed long         int64_t;
typedef signed long long    int128_t;

typedef int128_t            intmax_t;

typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long       uint64_t;
typedef unsigned long long  uint128_t;

typedef uint128_t           uintmax_t;

typedef int64_t     intptr_t;
typedef uint64_t    uintptr_t;

#define MAX(a, b)                   \
({                                  \
    __typeof__ (a) _a = (a);        \
    __typeof__ (b) _b = (b);        \
    _a > _b ? _a : _b;              \
})

#define MIN(a, b)                   \
({                                  \
    __typeof__ (a) _a = (a);        \
    __typeof__ (b) _b = (b);        \
    _a < _b ? _a : _b;              \
})

#define FLOOR(a, b)                  \
({                                  \
    __typeof__ (a) _a = (a);        \
    __typeof__ (b) _b = (b);        \
    (_a / _b);                      \
})


#define CEIL(a, b)                  \
({                                  \
    __typeof__ (a) _a = (a);        \
    __typeof__ (b) _b = (b);        \
    (_a / _b) + (_a % _b > 0);      \
})


#define ROUND_DOWN(x, n)    ((x) & ~((n) - 1))
#define ROUND_UP(x, n)      (((x) + n - 1) & ~((n) - 1))


#endif //TINY_OS_STDINT_H
