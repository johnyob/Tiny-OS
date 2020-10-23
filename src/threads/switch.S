########################################################################################################################
# - Alistair O'Brien - 7/09/2020 - University of Cambridge
########################################################################################################################
#      File:        switch.S
#      Environment: Tiny OS
#      Description: Contains the context switching logic for Tiny OS
########################################################################################################################

#include <threads/switch.h>
.altmacro
.macro save x, i, base_reg=sp
    sd \x, ((\i)*REG_SIZE)(\base_reg)
.endm
.macro load x, i, base_reg=sp
    ld \x, ((\i)*REG_SIZE)(\base_reg)
.endm


.macro save_callee_saved i, base_reg=sp
    save s\i, ((\i) + 1), \base_reg
.endm
.macro load_callee_saved i, base_reg=sp
    load s\i, ((\i) + 1), \base_reg
.endm


# Procedure:    switch_contexts
# ------------------------
# This procedure implements the context switching. When considering context switches, we must consider
# the kernel stack of each thread: TODO
#
.section .text
.global switch_contexts
switch_contexts:
    # We need to allocate space for the context on the kernel stack of the currently executing thread.
    # By our calcuations, the size of a context is 104 bytes.
    addi sp, sp, -104

    # We note that when a thread is executing, the pointer to it's context is null. Hence we 
    # store the stack pointer in a0 (a pointer to a pointer to a context), since dereferencing a0
    # and storing the pointer to the new context correctly stores the new context in a0. 
    sd sp, 0(a0)

    # We now store call the callee saved registers in our context, we do this with the help
    # of a useful macro (got love me some macros!) 
    # We store these registers first so we can use them as temporary registers when reading the 
    # csr satp register.   
    .set i, 0
    .rept NUM_CALLEE_SAVED_REGS
        save_callee_saved %i, sp
        .set i, i + 1
    .endr
    
    # Now we have to store our special cases :)
    save ra, 0, sp

    # At this point, we've now successfully stored the context of the currently executing thread. Good job! (have a biscuit)

    # To load a context, we perform the reverse of saving a context :) simples

    # Dereferencing a1 gives the pointer to the next context. Store this pointer as the stack pointer. 
    # This now means the stack pointer is pointing to the kernel stack of the next thread. 
    ld sp, 0(a1)

    load ra, 0, sp

    .set i, 0
    .rept NUM_CALLEE_SAVED_REGS
        load_callee_saved %i, sp
        .set i, i + 1
    .endr

    # Since we've now loaded the context, we can just ignore it by setting the sp to it's original point before the 
    # thread yeilded.
    addi sp, sp, 104

    # Set the next context to null
    sd zero, 0(a1)

    # We return a pointer to the previous context (the one that was stored on the stack)
    # Since a0 already stores a pointer to the pointer of the previous context, simply store a dereferenced value
    # of a0 in a0.
    ld a0, 0(a0)

    ret



.global __schedule_tail_entry
__schedule_tail_entry:
    # The top of the stack should contain a trap frame :)
    # Previous context pointer is stored in register a0
    # Since we have no previous caller, this procedure need-not restore the caller saved context

    # Page round down
    li t0, 4096 # PAGE_SIZE
    li t1, 1 # Required since subi doesn't exist...
    sub t0, t0, t1 # PAGE_SIZE - 1
    not t0, t0 # ~(PAGE_SIZE - 1)
    and a0, a0, t0 # a0 & ~(PAGE_SIZE - 1)

    # Now call schedule tail
    call __schedule_tail

    # We return here once schedule tail has finished.

    la ra, s_ret_trap
    # Set return address to s_ret_trap

    ret
