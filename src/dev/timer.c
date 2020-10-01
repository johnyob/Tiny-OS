////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 7/14/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        timer.c
//      Environment: Tiny OS
//      Description:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



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

extern void m_trap_vec();

#define CLINT_START         (0x2000000L)

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
static uint64_t mscratch[NCPU][5];

// The number of timer ticks since the OS booted.
static uint64_t ticks;

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

uint64_t timer_ticks() {
    // Disable all interrupts
    intr_state_t state = intr_disable();

    uint64_t t = ticks;

    // Return the interruptor to it's previous setting
    intr_state(state);
    return t;
}

void timer_handle_interrupt(UNUSED trap_frame_t* tf) {
    ticks++;
}