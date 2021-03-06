////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        stdlib.h
//      Environment: Tiny OS
//      Description: Contains some of the standard lib methods. Currently only contains abs (TODO: add more)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_STDLIB_H
#define TINY_OS_STDLIB_H

static inline int abs(int n) {
    return n < 0 ? -(n) : n;
}


#endif //TINY_OS_STDLIB_H
