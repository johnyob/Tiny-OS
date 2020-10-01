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

#include <debug.h>
#include <trap.h>

// QEMU location for UART MMIO address space
#define UART0 (0x10000000)

/***********************************************************************************************************************
 * UART Control Registers
 *
 * - RBR (Reciever Buffer Register) is a read only 8-bit register.
 * - THR (Transmitter Holding Register) is a write only 8-bit register. We note that
 *   RBR and THR have the same MMIO address, this is due to the fact that we can differentiate between
 *   the registers since one is read only and the other is write only. This design detail
 *   occurs in many MMIO address spaces for external controllers.
 *
 * See:
 */

#define RBR (UART0 + 0) // Reciever Buffer Register (read only)
#define THR (UART0 + 0) // Transmitter Holding Register (write only)
#define IER (UART0 + 1) // Interrupt Enable Register
#define IIR (UART0 + 2) // Intrrupt Ident Register (read only)
#define FCR (UART0 + 2) // FIFO control register (write only)
#define LCR (UART0 + 3) // Line control register
#define MCR (UART0 + 4) // MODEM control register
#define LSR (UART0 + 5) // Line status register
#define MSR (UART0 + 6) // MODEM Status Register

#define DDL (UART0 + 0)
#define DLM (UART0 + 1)

#define DR_MASK     (1 << 0)  // Data Ready bit (in LSR) mask
#define THR_MASK    (1 << 5)  // Transmitter holding register bit (in LSR) mask

void    uart_init();
uchar_t uart_getc();
void    uart_putc(uchar_t c);

void uart_handle_interrupt(UNUSED trap_frame_t* tf);

#endif //TINY_OS_UART_H
