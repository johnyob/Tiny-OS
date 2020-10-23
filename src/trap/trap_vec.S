########################################################################################################################
# - Alistair O'Brien - 7/09/2020 - University of Cambridge
########################################################################################################################
#      File:        trap_vec.S
#      Environment: Tiny OS
#      Description: Contains the trap vectors, which are the entry point for all traps in Tiny OS.
########################################################################################################################

########################################################################################################################
# Tiny OS Trap Vectors                                                                                                 #
########################################################################################################################
# In RISC-V, we have 3 priveleges; Machine, Supervisor and User. Tiny OS uses these privileges
# to partially infer the context of interrupts.
#
# By design, the Tiny OS kernel executes in Supervisor Mode. So we deduce that any interrupt / trap that
# occurs in supervisor mode is therefore caused by the kernel (or any external source that the kernel
# manages). So we define any interrupts/traps from supervisor mode to be "kernel" interrupts/traps.
#
# We do have the exception of the machine mode timer interrupts, which are handled by the CLINT procedures
# in interrupt.c. So we define any interrupts/traps from machine mode to be "timer" interrupts/traps.
#
# Similar to "kernel" traps, we have the concept of "user" traps. These are interrupts/traps that
# occur in user mode. (TODO)
########################################################################################################################

########################################################################################################################
# RISC-V Registers                                                                                                     #
########################################################################################################################
# RISC-V has two types of registers: general purpose registers and floating point registers,
# each using different instructions to load and store data in the registers.
#
# RISC-V has 32 base general purpose registers x0 - x31 (only 31 of them need to be saved, however we store all 32
# for convenience):
#   x0        zero    hardwired to zero
#   x1        ra      return address
#   x2        sp      stack pointer
#   x3        gp      global pointer
#   x4        tp      thread pointer
#   x5-7      t0-2    temp registers
#   x8        s0/fp   saved register / frame pointer
#   x9        x1      saved register
#   x10-11    a0-1    arguments / return values
#   x12-17    a2-7    arguments
#   x18-27    s1-11   saved registers
#   x28-31    t3-t6   temp registers
#
# RISC-V also has 32 floating point register f0 - f31:
#   f0-7      ft0-7   floating point temp registers
#   f8-9      fs0-1   floating point saved registers
#   f10-11    fa0-1   floating point arguments / return values
#   f12-17    fa2-7   floating point arguments
#   f18-27    fs2-11  floating point saved registers
#   f28-31    ft8-11  floating point temp registers
########################################################################################################################

#include <trap/trap.h>

# To do much of the heavy lifting for context switches, we will be using ASM macros.
# These macros are designed for saving an loading registers from a trap frame (see trap.h)

.altmacro

# General purpose register macros:
# The [save_gp] and [load_gp] macros load and save the ith general purpose register [xi]
# in the space reserved for the ith general purpose register in the trap frame with
# address [base_reg].

.macro save_gp i, base_reg
    sd x\i, ((\i)*REG_SIZE)(\base_reg)
.endm
.macro load_gp i, base_reg
    ld x\i, ((\i)*REG_SIZE)(\base_reg)
.endm

# Floating point register macros:
# The [save_fp] and [load_fp] macros load and save the ith floating point register [fi]
# in the space reserved for the ith floating point register in the trap frame with
# address [base_reg].

.macro save_fp i, base_reg
    fsd f\i, ((NUM_GP_REGS + (\i))*REG_SIZE)(\base_reg)
.endm
.macro load_fp i, base_reg
    fld f\i, ((NUM_GP_REGS + (\i))*REG_SIZE)(\base_reg)
.endm


.section .text

# All RISC-V direct trap vectors must have an address ending in 00, that is to say the address
# is a multiple of 4.
.align 4
.global m_trap_vec

m_trap_vec:
    csrrw t0, mscratch, t0

    # Store the contents of the general purpose registers t1, t2 and t3
    # into the dedicated mscratch area.
    sd t1, 16(t0)
    sd t2, 24(t0)
    sd t3, 32(t0)

    # We need to schedule the next timer interrupt
    # We achieve this by adding our interval to the value stored in mtimecmp
    ld t1, 0(t0)    # mtimecmp address
    ld t2, 8(t0)    # TIMER INTERVAL

    # Get the value stored in the address of mtimecmp.
    ld t3, 0(t1)

    add t3, t2, t3
    sd t3, 0(t1)

    # Send supervisor timer interrupt. This is done by setting the STIP (bit 5), software timer interrupt pending
    # bit in the MIP register to 1.
    li t1, 1
    slli t1, t1, 5

    csrw mip, t1

    ld t3, 32(t0)
    ld t2, 24(t0)
    ld t1, 16(t0)
    csrrw t0, mscratch, t0

    mret

.align 4
.global s_trap_vec

# Procedure:    s_trap_vec
# ------------------------
# This procedure implements the supervisor trap vector. Recall that all supervisor traps occur
# when executing kernel code. Due to this, we simply store the trap frame on the kernel stack.
# We then call s_trap (the supervisor trap handler). After s_trap finished executing, we
# restore the registers from the trap frame on the stack and pop it off the stack.

s_trap_vec:

    # We need to allocate space for our trap frame. From our calculations, the size of a trap frame is 544 bytes.
    addi sp, sp, -544

    # We now save all the general purpose registers on the stack.
    .set i, 0
    .rept NUM_GP_REGS
        save_gp %i, sp
        .set i, i + 1
    .endr

    # We now save all the floating point registers onto the stack. Note that the beginning of the
    # [fp_regs] array in our trap frame is 256(sp). This is accounted for NUM_GP_REGS in the macro
    # save_fp.
    .set i, 0
    .rept NUM_FP_REGS
        save_fp %i, sp
        .set i, i + 1
    .endr

    # With all the general purpose registers stored, we can now use them to store the csr values in the trap frame.
    csrr t0, sstatus
    sd t0, 512(sp)

    csrr t0, sepc
    sd t0, 520(sp)

    csrr t0, stval
    sd t0, 528(sp)

    csrr t0, scause
    sd t0, 536(sp)

    # We now call the s_trap handler in trap.c. s_trap takes a pointer
    # for the trap frame, so we need to move sp (the stack pointer, but also the pointer
    # to the beginning of our trap frame) into a0 (argument 0)
    mv a0, sp

    # Call s_trap :)
    call s_trap

.global s_ret_trap
s_ret_trap:
    # Once s_trap has finished executing, we'll return here.
    # We now simply perform the inverse operations to what we did above.

    # First write the values of the csrs back, using the temp gp register t0
    ld t0, 536(sp)
    csrw scause, t0

    ld t0, 528(sp)
    csrw stval, t0

    ld t0, 520(sp)
    csrw sepc, t0

    ld t0, 512(sp)
    csrw sstatus, t0

    # Now we save the floating point registers
    .set i, 0
    .rept NUM_FP_REGS
        load_fp %i, sp
        .set i, i + 1
    .endr

    .set i, 0
    .rept NUM_GP_REGS
        load_gp %i, sp
        .set i, i + 1
    .endr

    addi sp, sp, 544

    sret
