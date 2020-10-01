////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        trap.h
//      Environment: Tiny OS
//      Description: Contains the procedure prototypes for the main and init procedures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef TINY_OS_TRAP_H
#define TINY_OS_TRAP_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TRAPS                                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * To support traps, we need some way of saving the context of the process prior to the trap.
 * We use a "trap frame", which stores all the register values in [regs], [fp_regs],
 * and other necessary information for switching user-spaces etc.
 */
typedef struct trap_frame {
    // Integer registers:           0 - 255
    uint64_t regs[32];

    // Floating point registers:    256 - 511
    uint64_t fp_regs[32];

    // Status register:             512 - 519
    uint64_t status;

    // Exception program counter:   520 - 527
    uint64_t epc;

    // Trap value:                  528 - 535
    uint64_t tval;

    // Trap cause:                  536 - 543
    uint64_t cause;
} trap_frame_t;


void trap_init();

void s_trap(trap_frame_t* tf);
// void u_trap(trap_frame_t* tf);

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERRUPTS                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    INTR_U_SOFT     = CAUSE_USI,
    INTR_S_SOFT     = CAUSE_SSI,
    INTR_M_SOFT     = CAUSE_MSI,

    INTR_U_TIMER    = CAUSE_UTI,
    INTR_S_TIMER    = CAUSE_STI,
    INTR_M_TIMER    = CAUSE_MEI,

    INTR_U_EXT      = CAUSE_UEI,
    INTR_S_EXT      = CAUSE_SEI,
    INTR_M_EXT      = CAUSE_MEI
} intr_t;


typedef enum {
    EXT_INTR_UART0_IRQ      = 10,
    EXT_INTR_RTC_IRQ        = 11,
    EXT_INTR_VIRTIO_IRQ     = 1, // 1 to 8
    EXT_INTR_VIRTIO_COUNT   = 8,
    EXT_INTR_PCIE_IRQ       = 0x20, // 32 to 35
    EXT_INTR_VIRTIO_NDEV    = 0x35 // Arbitrary maximum number of interrupts
} ext_intr_t;

void intr_init();

typedef void intr_handler_t(trap_frame_t* tf);

void intr_register(intr_t intr, intr_handler_t* handler);
void ext_intr_register(ext_intr_t intr, intr_handler_t* handler);


#endif //TINY_OS_TRAP_H
