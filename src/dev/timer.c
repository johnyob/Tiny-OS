////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 7/14/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        timer.c
//      Environment: Tiny OS
//      Description:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>

#include <param.h>
#include <riscv.h>
#include <debug.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

#include <trap/interrupt.h>

#include <threads/thread.h>

#include <dev/timer.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLINT (Core Local Interruptor)                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The CLINT is used to manage and control software and timer interrupts for each hart.
// For QEMU's RISC-V virt architecture, the CLINT is a block of memory-mapped control and status registers
// including the msip, mtimecmp and mtime registers.
//
// For our purposes, we will be using the CLINT to control the timer, using the mtimecmp and mtime registers.
// For each hart h_i, there is a single mtimecmp register. The mtimecmp register is the timer compare register, which
// causes a timer interrupt to be sent to hart h_i when the mtime register contains a value greater than or equal to the
// value in the mtimecmp register.
//
// To handle the interrupt, we must overwrite the mtimecmp, this notifies the CLINT.
//
// We note that by the design of Tiny OS and RISC-V, all CLINT code will execute in
// machine mode.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CLINT_START             (0x2000000L)
#define CLINT_SIZE              (0x10000L)

/*
 * The CLINT controls the timer using a series of mtimecmp registers, timer compare registers.
 * An interrupt is sent when the mtime register contains a value greater than or equal to
 * the value in the mtimecmp register.
 * Each mtimecmp register consists of 8-bytes with a base address of 0x2004000.
 */
#define CLINT_MTIMECMP_BASE     (CLINT_START + 0x4000)
#define CLINT_MTIMECMP(id)      (CLINT_MTIMECMP_BASE + 8 * id)

#define CLINT_MTIME             (CLINT_START + 0xbff8)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNAL CLINT METHODS                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * To handle timer interrupts, we somewhere to store the mtimecmp mmio address and the
 * timer interval. We also some additional space to save the values stored in the
 * temporary registers that we use during the timer interrupt vector. Our assembly code
 * only uses the registers t1, t2 and t3. Hence our mscratch area requires 5 8 byte entries.
 */
static uint64_t mscratch[NUM_HART][5];

// The number of timer ticks since the OS booted.
static volatile uint64_t ticks;

void timer_init() {
    uint64_t hartid = r_hartid();

    uint64_t* scratch = &mscratch[hartid][0];

    scratch[0] = CLINT_MTIMECMP(hartid);
    scratch[1] = TIMER_INTERVAL;

    *(uint64_t*)scratch[0] = *(uint64_t*)CLINT_MTIME + TIMER_INTERVAL;

    w_mtvec(MTVEC((uintptr_t)m_trap_vec, MTVEC_MODE_DIRECT));
    w_mscratch((uintptr_t)scratch);

    w_mstatus(r_mstatus() | MSTATUS_MIE);
    w_mie(r_mie() | MIE_MTIE);

}

/*
 * Procedure:   timer_vm_init
 * --------------------------
 * This procedure performs the kernel virtual memory mapping required during initialization.
 *
 */
void timer_vm_init() {
    kmap(CLINT_START, CLINT_START, CLINT_SIZE, PTE_R | PTE_W);
    info("clint: \t%#p -> %#p\n", CLINT_START, CLINT_START + CLINT_SIZE);
}

uint64_t timer_ticks() {
    // Disable all interrupts
    intr_state_t state = intr_disable();

    uint64_t t = ticks;

    // Return the interruptor to it's previous setting
    intr_set_state(state);
    return t;
}

uint64_t timer_elapsed(uint64_t then) {
    uint64_t t = timer_ticks();

    assert(t >= then);
    return t - then;
}

void timer_sleep(uint64_t t) {
    uint64_t ticks0 = timer_ticks();

    assert(intr_get_state() == INTR_ON);

    info("%d thread\n", thread_current()->tid);
    info("%d ticks0\n", ticks0);

    while (timer_elapsed(ticks0) < t) {
        thread_yield();
        if (timer_elapsed(ticks0) > 0) info("Yay!\n");
    }
}

void timer_handle_interrupt(UNUSED trap_frame_t* tf) {
    ticks++;
    scheduler_tick();
}