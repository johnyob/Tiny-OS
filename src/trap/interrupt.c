////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 7/11/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        interrupt.c
//      Environment: Tiny OS
//      Description: Interrupt implements methods for dealing with external interrupts,
//                   specifically using the PLIC (Platform Local Interrupt Controller).
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>

#include <riscv.h>
#include <debug.h>
#include <param.h>

#include <trap/trap.h>

#include <dev/plic.h>
#include <dev/timer.h>

#include <trap/interrupt.h>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERRUPTS                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    INTR_U_SOFT     = CAUSE_USI,
    INTR_S_SOFT     = CAUSE_SSI,
    INTR_M_SOFT     = CAUSE_MSI,

    INTR_U_TIMER    = CAUSE_UTI,
    INTR_S_TIMER    = CAUSE_STI,
    INTR_M_TIMER    = CAUSE_MTI,

    INTR_U_EXT      = CAUSE_UEI,
    INTR_S_EXT      = CAUSE_SEI,
    INTR_M_EXT      = CAUSE_MEI
} intr_t;

void intr_init() {

    // Initialize PLIC (Platform Local Controller)
    plic_vm_init();
    plic_init();

    // Initialize timer (CLINT) virtual memory
    timer_vm_init();

}


void s_intr_handler(trap_frame_t* tf) {
    intr_t intr = SCAUSE_EXCEPTION(tf->cause);

    switch (intr) {
        case INTR_S_TIMER:
            timer_handle_interrupt(tf);
            break;
        case INTR_S_EXT:
            plic_handle_interrupt(tf);
            break;
        default:
            dump_trap_frame(tf);
            panic("Unexpected interrupt.\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERRUPT STATE                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


intr_state_t intr_get_state() {
    return (r_sstatus() & SSTATUS_SIE_MASK) >> 1;
}

void intr_set_state(intr_state_t state) {
    if (state == INTR_ON) {
        intr_enable();
    } else {
        intr_disable();
    }
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

