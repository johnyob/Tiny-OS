########################################################################################################################
# - Alistair O'Brien - 6/18/2020 - University of Cambridge
########################################################################################################################
#      File:        boot.S
#      Environment: Tiny OS
#      Description: The bootloader is the entry point for the kernel. It clears the BSS section for the kernel,
#                   sets up the kernel stack and then calls init (a C procedure)
########################################################################################################################

.section .text

# Define start procedure for the bootloader
.global _start
_start:
    # We initially want to disable paging (otherwise we wouldn't be able to clear the BSS section
    # on account of not knowing the *virtual* address). We do however know the physical addresses of the BSS section
    csrw satp, zero

    # TEMP: For ease of development, we will be disabling any additional harts (hardware threads)
    csrr t0, mhartid
    bnez t0, 3f

    # We now zero out the BSS section
    # Load the address of the BSS start and end into t0 and t1 respectively.
    la t0, __BSS_START
    la t1, __BSS_END

    # If t0 >= t1 (unsigned) then jump to 2
    bgeu t0, t1, 2f
1:
    # Zero 8 bytes (since we use the sd instruction, store double, which is an 8 byte type)
    sd zero, (t0)
    # Add 8 to the address stored in t0
    addi t0, t0, 8
    # If t0 < t1 (unsigned) then jump to 1
    bltu t0, t1, 1b
2:
    # We have now cleared the BSS section :)

    # We now initialize the stack. The stack grows down from the end of the stack,
    # so we must initialize the stack pointer to the end of kernel stack.
    la sp, __KERNEL_STACK_END

    # Call init. This is our kernel's setup procedure before calling main.
    # It is written in C which makes life easier :)
    call init
3:
    wfi
    j 3b
