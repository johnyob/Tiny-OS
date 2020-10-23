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
#include <lib/stdio.h>
#include <lib/string.h>

#include <param.h>
#include <riscv.h>
#include <debug.h>

#include <dev/uart.h>
#include <dev/timer.h>

#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/malloc.h>

#include <trap/trap.h>

#include <threads/thread.h>

#include <main.h>

uint8_t kernel_stack[NUM_HART * 4096] __attribute__((section(".stack")));


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
    w_mepc((uintptr_t)main);

    // We want to clear all interrupt options (only allowing exceptions)
    uint64_t mie = r_mie();
    mie &= ~(MIE_MTIE | MIE_MSIE | MIE_MEIE);
    w_mie(mie);

    // To access the hartid from supervisor mode, we store it in the tp register
    w_hartid(r_mhartid());

    // Initialize the timer (CLINT)
    timer_init();

    asm("mret");
}

/*
 * Procedure:   main
 * -----------------
 *
 */
void main() {

    // For testing, this always holds true
    if (r_hartid() == 0) {
        thread_init();
        thread_hart_init();

        // UART Initialization
        uart_init();
        printf("Hello World :)\n");

        info("Threads initialized.\n");
        info("UART initialized.\n");

        info("PMM initializing...\n");
        pmm_init();
        info("PMM initialized.\n");


        info("VMM initializing...\n");
        vmm_init();

        // Initialize thread and uart virtual memory (vm) here (special since uart is used for logging)
        uart_vm_init();
        thread_vm_init();

        vmm_hart_init();
        info("VMM initialized.\n");

        info("malloc initializing...\n");
        malloc_init();
        info("malloc initialized.\n");


        info("Traps initializing...\n");
        trap_init();
        trap_hart_init();
        info("Traps initialized.\n");

        scheduler_start();


        for (;;) {
            info("Kernel thread now sleeping...\n");
            timer_sleep(10000000);
        }
    }

    while (true);
}