////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/22/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        symbols.h
//      Environment: Tiny OS
//      Description: Contains the symbols from the linker e.g. TEXT_START, TEXT_END, etc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_SYMBOLS_H
#define TINY_OS_SYMBOLS_H

#include <lib/stdint.h>

// Include external linker symbols for addresses
// Apparently these symbols store the values of the addresses,
// not the addresses themselves, so we define macros
// which contain the addresses.

extern uint8_t __TEXT_START;
extern uint8_t __TEXT_END;
extern uint8_t __RODATA_START;
extern uint8_t __RODATA_END;
extern uint8_t __DATA_START;
extern uint8_t __DATA_END;
extern uint8_t __BSS_START;
extern uint8_t __BSS_END;
extern uint8_t __STACK_START;
extern uint8_t __STACK_END;
extern uint8_t __HEAP_START;
extern uint8_t __MEMORY_END;
extern uint8_t __HEAP_SIZE;

#define TEXT_START          (uintptr_t)(&__TEXT_START)
#define TEXT_END            (uintptr_t)(&__TEXT_END)
#define RODATA_START        (uintptr_t)(&__RODATA_START)
#define RODATA_END          (uintptr_t)(&__RODATA_END)
#define DATA_START          (uintptr_t)(&__DATA_START)
#define DATA_END            (uintptr_t)(&__DATA_END)
#define BSS_START           (uintptr_t)(&__BSS_START)
#define BSS_END             (uintptr_t)(&__BSS_END)
#define STACK_START         (uintptr_t)(&__STACK_START)
#define STACK_END           (uintptr_t)(&__STACK_END)
#define HEAP_START          (uintptr_t)(&__HEAP_START)
#define MEMORY_END          (uintptr_t)(&__MEMORY_END)
#define HEAP_SIZE           (uint64_t)(&__HEAP_SIZE)

#endif //TINY_OS_SYMBOLS_H
