////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        uart.h
//      Environment: Tiny OS
//      Description: Contains the prototypes for uart_init, uart_getc, uart_putc and uart_handle_interrupt.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_UART_H
#define TINY_OS_UART_H

#include <lib/ctype.h>
#include <trap/trap.h>

// QEMU location for UART MMIO address space
#define UART0 (0x10000000)

void    uart_init();
void    uart_vm_init();

uchar_t uart_getc();
void    uart_putc(uchar_t c);

void uart_handle_interrupt(UNUSED trap_frame_t* tf);

#endif //TINY_OS_UART_H
