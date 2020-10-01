////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/19/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        uart.c
//      Environment: Tiny OS
//      Description: Contains methods for interacting with QEMU's UART controller which uses MMIO registers
//                   We use polling for writing and reading characters from UART (and will eventually use external
//                   interrupts for reading)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/ctype.h>
#include <lib/stdint.h>
#include <lib/stdarg.h>
#include <lib/stdio.h>

#include <debug.h>

#include <uart.h>


/*
 * Procedure:   mmio_write
 * -----------------------
 * This inline procedure writes [c] to the register with address [reg]
 *
 * @uint64_t reg:   The 64-bit address of the register.
 * @uchar_t c:      The character (or byte) that is to be written to the register.
 *
 */
static inline void mmio_write(uint64_t reg, uchar_t c) {
    *(volatile uchar_t*)reg = c;
}

/*
 * Function:   mmio_read
 * ---------------------
 * This inline function returns the contents of the register with address [reg]
 *
 * @uint64_t reg:   The 64-bit address of the register.
 *
 * @returns:        The value / contents of the register with address [reg]
 *
 */
static inline uchar_t mmio_read(uint64_t reg) {
    return *(volatile uchar_t*)reg;
}

/*
 * Procedure:   uart_init
 * ----------------------
 * This procedure performs the initialization for QEMU's UART controller.
 * From the inspection of the riscv64-virt.dts, we note that the UART controller's MMIO section
 * has a base address of 0x10000000 and a length/limit of 0x100.
 *
 * We also note that it's clock frequency is 0x384000 and is compatible with ns16550a.
 * To initialize this type of controller, we first set the baud rate using the
 * divisor mechanism.
 *
 * We then set the word length and enable FIFO (first in, first out).
 *
 */
void uart_init() {
    // Disable UART interrupts.
    mmio_write(IER, 0x00);

    // Set the DLAB (divisor latch access bit) to 1 (bit 7)
    // This allows us to set the baud rate.
    mmio_write(LCR, 0x80);

    // According to the specification, the divisor is given by
    //      divisor = ceil( (clock_hz) / (2^4 * baud_rate) )
    // From our dts file, we note that clock_hz = 3686400 Hz.
    // We define our baud_rate as baud_rate = 2^15 = 32768.
    // Hence divisor = ceil ( 3686400  / 2^19 ) = ceil ( 7.03125 ) = 8
    // The divisor is 16-bits, with the least significant in DDL
    // and the most significant in DLM.

    mmio_write(DDL, 0x08);
    mmio_write(DLM, 0x00);

    // We now set word length to 8 bit with no parity
    // and unset the DLAB bit.
    mmio_write(LCR, 0x03);

    // and enable the FIFO, which is bit 0 of FCR.
    mmio_write(FCR, 0x01);

    // Enable reciever buffer interrupt, bit 0 of the IER.
    mmio_write(IER, 0x01);
}

/*
 * Function:   uart_getc
 * ---------------------
 * This function returns a character if one is sent by the UART controller.
 * We do this by polling the controller until the data ready bit (in the LSR register)
 * is set.
 *
 * Once the data ready bit is set, we simply read the receiver buffer register (RBR).
 *
 * @returns:    The character stored in the RBR register.
 *              If no character is sent via the device controller
 *              then procedure hangs (due to the nature of polling...)
 *
 */
uchar_t uart_getc() {
    while ((mmio_read(LSR) & DR_MASK) != 0) ;
    return mmio_read(RBR);
}


/*
 * Procedure:   uart_putc
 * ----------------------
 * This function writes a character in the transmitter holding register of the UART controller
 * if and only if the transmitter holding ready bit is set. Otherwise we'd be overwriting data
 * that is currently being sent :(
 *
 * @uchar_t c:  The character that is to be sent by the UART controller.
 *
 */
void uart_putc(uchar_t c) {
    while ((mmio_read(LSR) & THR_MASK) == 0) ;
    mmio_write(THR, c);
}

// TEMP vprintf implementation
// TODO: write console driver :)

static inline void buf_putc(char c, void* buf) {
    int* n = (int*)buf;

    uart_putc(c);
    (*n)++;
}

int vprintf(const char* format, va_list va) {

    int n = 0;
    __vprintf(format, va, buf_putc, &n);
    return n;
}

void uart_handle_interrupt(UNUSED trap_frame_t* tf) {
    uchar_t c = mmio_read(RBR);
    uart_putc(c);
}