////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        trap.h
//      Environment: Tiny OS
//      Description: Contains the procedure prototypes for the main and init procedures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef TINY_OS_TRAP_H
#define TINY_OS_TRAP_H

// To calculate byte offsets, we need to know the number of bytes per a register.
// In QEMU's virt RISC-V 64, the registers consist of 8 bytes.
#define REG_SIZE            8

#define NUM_GP_REGS         32
#define NUM_FP_REGS         32


#ifndef __ASSEMBLER__

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
    uint64_t regs[NUM_GP_REGS];

    // Floating point registers:    256 - 511
    uint64_t fp_regs[NUM_FP_REGS];

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
void trap_hart_init();

// Trap handlers

void s_trap(trap_frame_t* tf);
// void u_trap(trap_frame_t* tf);

void dump_trap_frame(const trap_frame_t* tf);

// Trap vectors

void s_trap_vec(void);
void s_ret_trap(void);

void m_trap_vec(void);

#endif


#endif //TINY_OS_TRAP_H
