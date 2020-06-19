////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        main.c
//      Environment: Tiny OS
//      Description: Contains the init and main methods. The init method executes in machine mode and does some
//                   assorted stuff and then quickly jumps to main.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>
#include <lib/stdbool.h>

#include <riscv.h>
#include <main.h>

/*
 * Procedure:   init
 * -----------------
 * The kernel init procedure executes in machine-mode privilege. It's purpose
 * is to perform the necessary steps in machine-mode to quickly switch to supervisor
 * mode.
 *
 * The only way to return to a lower privilege level is to use the mret instruction.
 * To switch to supervisor mode and call main, we make it appear as an exception as
 * occurred in main, which is handled in init. Hence the epc will be set to main
 * and we can use our machine-mode privileges to setup the csrs s.t we return to supervisor
 * mode.
 *
 */
void init() {
    // Read the mstatus register. We want to set the previous privilege to supervisor
    // so when we call mret, we switch to supervisor mode.
    uint64_t mstatus = r_mstatus();

    // We use the MPP mask to ensure we don't modify any other bits and zero the MPP bit
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;

    // Write the modified contents back into the mstatus register :)
    w_mstatus(mstatus);

    // We also want to delegate all interrupts and exceptions to supervisor mode
    // since all stuff is handled in supervisor mode :)
    w_medeleg(0xffff);
    w_mideleg(0xffff);

    // We set the mepc to main, so when we call mret we begin the execution of main
    w_mepc((uint64_t)main);

    // Enable previous interrupts (in supervisor mode)
    w_mstatus(r_mstatus() | MSTATUS_MPIE);

    // We want to clear all interrupt options (only allowing exceptions)
    uint64_t mie = r_mie();
    mie &= ~(MIE_MTIE | MIE_MSIE | MIE_MEIE);
    w_mie(mie);

    // To access the hartid from supervisor mode, we store it in the tp register
    w_hartid(r_mhartid());

    asm("mret");
}

/*
 * Procedure:   main
 * -----------------
 *
 */
void main() {


    while (true);
}