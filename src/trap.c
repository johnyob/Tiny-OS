////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 7/11/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        trap.c
//      Environment: Tiny OS
//      Description: trap implements the trap handlers for the machine, supervisor and user privileges.
//                   The machine trap handler deals with timer interrupts, the supervisor trap handler
//                   deals with any traps that occur when executing kernel code and the user handler
//                   deals with any traps that occur when executing user code e.g. system calls, etc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdbool.h>

#include <debug.h>
#include <riscv.h>
#include <interrupt.h>

#include <trap.h>

extern void s_trap_vec();

/*
 * Procedure:   trap_init
 * ----------------------
 * To enable trap (exceptions and interrupts) handling, we need to set the SIE (Supervisor Interrupt Enable)
 * bit in the sstatus register and enable the timer, software and external interrupts in the
 * sie register.
 * We note that exceptions are always enabled, hence no modifications to exception registers
 * are required.
 */
void trap_init() {
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    w_sie(r_sie() | SIE_STIE | SIE_SSIE | SIE_SEIE);
}

/*
 * Procedure:   trap_hart_init
 * ---------------------------
 * To enable trap handling, we first have to store the address of trap handler for the kernel (s_trap)
 * in the stvec (supervisor trap vector) register using the direct trap mode.
 */
void trap_hart_init() {
    w_stvec(STVEC((uintptr_t)s_trap_vec, STVEC_MODE_DIRECT));
}


/*
 * Procedure:   s_trap
 * -------------------
 *
 */
void s_trap(trap_frame_t* tf) {

    // Determine whether the trap is caused by an external interrupt.
    bool is_interrupt = SCAUSE_INTERRUPT(tf->cause);

    // We must assert that s_trap is only called if the previous privilege is Supervisor.
    assert((tf->status & SSTATUS_SPP_MASK) != 0);

    if (!is_interrupt) {

    } else {
        // Interrupts.
        switch (cause) {
            case CAUSE_SEI:
                plic_handle_interrupt(tf);
                break;
            case CAUSE_SSI:
                timer_handle_interrupt(tf);
                break;
            default:
                panic(
                    "Unhandled interrupt. Hart: %d, status: %#x, epc: %p, cause: %d, tval: %#x.\n",
                    r_hartid(), tf->status, tf->epc, cause, tf->tval
                );
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXCEPTIONS                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    EXC_INST_ADDR_MISALIGNED    = CAUSE_INST_ADDR_MISALIGNED,
    EXC_INST_ACCESS_FAULT       = CAUSE_INST_ACCESS_FAULT,
    EXC_INST_PAGE_FAULT         = CAUSE_INST_PAGE_FAULT,

    EXC_LOAD_ADDR_MISALIGNED    = CAUSE_LOAD_ADDR_MISALIGNED,
    EXC_LOAD_ACCESS_FAULT       = CAUSE_LOAD_ACCESS_FAULT,
    EXC_LOAD_PAGE_FAULT         = CAUSE_LOAD_PAGE_FAULT,

    EXC_STORE_ADDR_MISALIGNED   = CAUSE_STORE_ADDR_MISALIGNED,
    EXC_STORE_ACCESS_FAULT      = CAUSE_STORE_ACCESS_FAULT,
    EXC_STORE_PAGE_FAULT        = CAUSE_STORE_PAGE_FAULT,

    EXC_U_ECALL                 = CAUSE_UECALL,
    EXC_S_ECALL                 = CAUSE_SECALL,
    EXC_M_ECALL                 = CAUSE_MECALL,

    EXC_ILLEGAL_INST            = CAUSE_ILLEGAL_INST,
    EXC_BREAKPOINT              = CAUSE_BREAKPOINT
} exc_t;

void s_exc_handler(trap_frame_t* tf) {
    exc_t exc = SCAUSE_EXCEPTION(tf->cause);

    // Exception
    switch (exc) {
        case EXC_INST_ADDR_MISALIGNED:
        case EXC_LOAD_ADDR_MISALIGNED:
        case EXC_STORE_ADDR_MISALIGNED:
            // An address is said to be misaligned iff the address is not divisible by the
            // word length e.g. the address 16 for ld is aligned (since it's divisible by 8)
            // and the address 11 is misaligned.
            panic("Address misaligned. Hart: %d, epc: %p, tval: %#x.\n", r_hartid(), tf->epc, tf->tval);
            break;
        case EXC_INST_PAGE_FAULT:
        case EXC_LOAD_PAGE_FAULT:
        case EXC_STORE_PAGE_FAULT:
            // A page fault occurs when a virtual address is dereferenced and it's unmapped
            // in the page table.
            panic("Page fault. Hart: %d, epc: %p, tval: %#x.\n", r_hartid(), tf->epc, tf->tval);
            break;
        case EXC_INST_ACCESS_FAULT:
        case EXC_LOAD_ACCESS_FAULT:
        case EXC_STORE_ACCESS_FAULT:
            // Access fault, this occurs when the current privilege doesn't satisfy the permission bits
            // of a virtual address.
            panic("Access fault. Hart: %d, epc: %p, tval: %#x.\n", r_hartid(), tf->epc, tf->tval);
            break;
        case EXC_UECALL:
        case EXC_SECALL:
        case EXC_MECALL:
            // Environment calls, this occurs when the RISC-V special ecall instruction is executed.
            panic(
                    "E-call. Hart: %d, privilege: %d, epc: %p, tval: %#x.\n",
                    r_hartid(), cause - CAUSE_UECALL, tf->epc, tf->tval
            );
            break;
        case EXC_ILLEGAL_INST:
            // Illegal instruction executed.
            panic("Illegal instruction. Hart: %d, epc: %p, tval: %#x.\n", r_hartid(), tf->epc, tf->tval);
            break;
        case EXC_BREAKPOINT:
            // Breakpoint encountered.
            panic("Breakpoint encountered. Hart: %d, epc: %p, tval: %#x.\n", r_hartid(), tf->epc, tf->tval);
            break;
        default:
            panic(
                    "Unhandled exception. Hart: %d, status: %#x, epc: %p, cause: %d, tval: %#x.\n",
                    r_hartid(), tf->status, tf->epc, cause, tf->tval
            );
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERRUPT STATE                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

intr_state_t intr_get_state() {
    return r_sstatus() & SSTATUS_SIE_MASK;
}

intr_state_t intr_set_state(intr_state_t state) {
    return state == INTR_ON ? intr_enable() : intr_disable();
}

intr_state_t intr_enable() {
    intr_state_t prev_state = intr_get_state();
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    return prev_state;
}

intr_state_t intr_disable() {
    intr_state_t prev_state = intr_get_state();
    w_sstatus(r_sstatus() & ~SSTATUS_SIE_MASK);
    return prev_state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERRUPTS                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void intr_init() {

    // Initialize PLIC (Platform Local Controller)
    plic_init();

}

void intr_dump_frame(const trap_frame_t* tf) {

}

void s_intr_handler(trap_frame_t* tf) {
    intr_t intr = SCAUSE_EXCEPTION(tf->cause);

    intr_handler_t* handler = iht[intr];
    if (handler != null) {
        handler(tf);
    } else {
        intr_dump_frame(tf);
        panic("Unexpected interrupt.\n");
    }
}






