////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 7/11/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        interrupt.h
//      Environment: Tiny OS
//      Description: Contains the procedure prototypes for the PLIC initialize and handle interrupt
//                   and the CLINT initialize and handle interrupt.
//                   Also contains the START and SIZE mmio symbols for the PLIC and CLINT.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_INTERRUPT_H
#define TINY_OS_INTERRUPT_H

#include <trap/trap.h>

void intr_init();

void s_intr_handler(trap_frame_t* tf);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERRUPT STATE                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * In Tiny OS it is often the case that some operations cannot be interrupted by an interrupt (an asynchronous
 * trap). So we must provide a mechanism that allows kernel code to disable interrupts, but we must
 * consider the case where interrupts are disabled and we call the following code:
 *      intr_disable();
 *      ...
 *      intr_enable();
 * In this case, interrupts should not be enabled when intr_enable is called. To implement this behavior we use
 * 'interrupt states'.
 */
typedef enum {
    INTR_OFF,
    INTR_ON
} intr_state_t;

intr_state_t intr_get_state();
void intr_set_state(intr_state_t state);

intr_state_t intr_enable();
intr_state_t intr_disable();

#endif //TINY_OS_INTERRUPT_H
