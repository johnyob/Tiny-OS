////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/18/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        riscv.h
//      Environment: Tiny OS
//      Description: Contains a collection of static inline methods for accessing RISC-V registers with associated
//                   macros (formatted according to the RISC-V specification).
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_RISCV_H
#define TINY_OS_RISCV_H

#include <lib/stdint.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACHINE LEVEL CSRS (Control and Status Registers)                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/***********************************************************************************************************************
 * The mhartid CSR (Control and Status Register) is a 64-bit read-only register containing the identifier (uint64_t)
 * of the hardware thread (hart) currently executing the program.
 *
 * Note that hart ids need not be numbered contiguously in a multi-hart system, but
 * at least one hart must have the id of 0.
 *
 */

static inline uint64_t r_mhartid() {
    uint64_t x;
    asm ("csrr %0, mhartid" : "=r" (x));
    return x;
}

/*
 * To access the hartid frequently in supervisor mode, tiny os will store the hartid in the tp, thread pointer register.
 * (This idea is inspired by xv6-riscv's decision to store the hartid in tp register)
 */

static inline uint64_t r_hartid() {
    uint64_t x;
    asm ("mv %0, tp" : "=r" (x));
    return x;
}

static inline void w_hartid(uint64_t x) {
    asm ("mv tp, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The mstatus CSR is a 64-bit read and write register with the following format:
 *
 * 63   62     35         33         31     22    21   20    19    18    17    16         14        12         10
 * +----+------+----------+----------+------+-----+----+-----+-----+-----+------+---------+---------+----------+
 * | SD | WPRI | SXL[1:0] | UXL[1:0] | WPRI | TSR | TW | TVM | MXR | SUM | MPRV | XS[1:0] | FS[1:0] | MPP[1:0] | ...
 * +----+------+----------+----------+------+-----+----+-----+-----+-----+------+---------+---------+----------+
 *   1    27        2          2        9      1    1    1      1     1     1       2         2          2
 *
 *      10     8     7      6      5      4      3     2      1     0
 *      +------+-----+------+------+------+------+-----+------+-----+-----+
 *  ... | WPRI | SPP | MPIE | WPRI | SPIE | UPIE | MIE | WPRI | SIE | UIE |
 *      +------+-----+------+------+------+------+-----+------+-----+-----+
 *          2      1     1       1      1      1     1      1     1     1
 *
 * Interrupt Enable Bits
 * The MIE, SIE and UIE are interrupt bits for the Machine, Supervisor and User privileges. If a hart is at privilege
 * x and xIE = 1, then interrupts are enabled for that privilege.
 *
 * Previous Privilege Bits:
 * To support nested trap handling, the mstatus CSR contains MPP, SPP, MPIE and SPIE; machine previous privilege,
 * supervisor previous privilege, machine previous interrupt enable and supervisor previous interrupt enable bits.
 *
 * MPP is 2 bits, SPP is 1 bit (supervisor or user) and UPP is implicitly 0.
 *
 * When a trap occurs in privilege y and moves to privilege x to handle it, the xPIE is set to yIE
 * and xIE is set to 0 and xPP is set to y.
 *
 */

#define MSTATUS_MPP_MASK    (3L << 11)
#define MSTATUS_MPP_M       (3L << 11)
#define MSTATUS_MPP_S       (1L << 11)
#define MSTATUS_MMP_U       (0L << 11)

#define MSTATUS_SPP_MASK    (1L << 8)
#define MSTATUS_SPP_S       (1L << 8)
#define MSTATUS_SPP_U       (0L << 8)

#define MSTATUS_MPIE_MASK   (1L << 7)
#define MSTATUS_MPIE        (1L << 7)

#define MSTATUS_SPIE_MASK   (1L << 5)
#define MSTATUS_SPIE        (1L << 5)

#define MSTATUS_UPIE_MASK   (1L << 4)
#define MSTATUS_UPIE        (1L << 4)

#define MSTATUS_MIE_MASK    (1L << 3)
#define MSTATUS_MIE         (1L << 3)

#define MSTATUS_SIE_MASK    (1L << 1)
#define MSTATUS_SIE         (1L << 1)

#define MSTATUS_UIE_MASK    (1L << 0)
#define MSTATUS_UIE         (1L << 0)

static inline uint64_t r_mstatus() {
    uint64_t x;
    asm ("csrr %0, mstatus" : "=r" (x));
    return x;
}

static inline void w_mstatus(uint64_t x) {
    asm ("csrw mstatus, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The mtvec register is a 64-bit read/write register that holds the trap vector config
 * It consists of a base address for the trap vector (BASE) and a trap vector mode (mode)
 *
 * The mtvec has the following format:
 *      63                   2
 *      +--------------------+------------+
 *      |        BASE        |    MODE    |
 *      +--------------------+------------+
 *               62                2
 *
 */

#define MTVEC_MODE_MASK     (0x3)
#define MTVEC_BASE_MASK     (~MTVEC_MODE_MASK)

#define MTVEC_MODE_DIRECT   0
#define MTVEC_MODE_VECTORED 1

#define MTVEC_BASE(mtvec)   (mtvec & MTVEC_BASE_MASK)
#define MTVEC_MODE(mtvec)   (mtvec & MTVEC_MODE_MASK)

#define MTVEC(base, mode)   (base | (mode & MTVEC_MODE_MASK))

static inline uint64_t r_mtvec() {
    uint64_t x;
    asm ("csrr %0, mtvec" : "=r" (x));
    return x;
}

static inline void w_mtvec(uint64_t x) {
    asm ("csrw mtvec, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The machine trap delegation registers, medeleg and mideleg (machine exception and machine interrupt)
 * are read/write registers.
 *
 * These registers are used to redirect certain traps from machine mode to a lower privilege.
 *
 * medeleg and mideleg contains a bit for each exception (see line ??) and interrupt (see line ??)
 *
 */

static inline uint64_t r_medeleg() {
    uint64_t x;
    asm ("csrr %0, medeleg" : "=r" (x));
    return x;
}

static inline void w_medeleg(uint64_t x) {
    asm ("csrw medeleg, %0" : : "r" (x));
}

static inline uint64_t r_mideleg() {
    uint64_t x;
    asm ("csrr %0, mideleg" : "=r" (x));
    return x;
}

static inline void w_mideleg(uint64_t x) {
    asm ("csrw mideleg, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The mip register is a 64-bit read/write register containing details about pending interrupts.
 *
 * It has the following format:
 *
 * 63     11     10      9     8      7      6      5      4      3      2      1      0
 * +------+------+------+------+------+------+------+------+------+------+------+------+------+
 * | WPRI | MEIP | WPRI | SEIP | UEIP | MTIP | WPRI | STIP | UTIP | MSIP | WPRI | SSIP | USIP |
 * +------+------+------+------+------+------+------+------+------+------+------+------+------+
 *    52     1      1      1      1      1      1      1       1     1       1      1      1
 *
 * Interrupt Pending Bits:
 * The MTIP, STIP, UTIP bits correspond to timer interrupt pending bits for machine, supervisor and user
 * interrupt timers.
 *
 * The MEIP is a read-only bit, which indicates whether there is a machine-mode external interrupt pending.
 * It is set and cleared by the PLIC (Platform Level Interrupt Controller).
 * The SEIP and UEIP are the supervisor and user mode equivalents, but they can also be written to in higher privilege
 * modes.
 *
 * The MTIP and MSIP are machine timer interrupt pending and machine software interrupt pending bits.
 * They supervisor and user mode equivalents.
 *
 */

#define MIP_MEIP_MASK   (1L << 11)
#define MIP_MEIP        (1L << 11)

#define MIP_SEIP_MASK   (1L << 9)
#define MIP_SEIP        (1L << 9)

#define MIP_UEIP_MASK   (1L << 8)
#define MIP_UEIP        (1L << 8)

#define MIP_MTIP_MASK   (1L << 7)
#define MIP_MTIP        (1L << 7)

#define MIP_STIP_MASK   (1L << 5)
#define MIP_STIP        (1L << 5)

#define MIP_UTIP_MASK   (1L << 4)
#define MIP_UTIP        (1L << 4)

#define MIP_MSIP_MASK   (1L << 3)
#define MIP_MSIP        (1L << 3)

#define MIP_SSIP_MASK   (1L << 1)
#define MIP_SSIP        (1L << 1)

#define MIP_USIP_MASK   (1L << 0)
#define MIP_USIP        (1L << 0)

static inline uint64_t r_mip() {
    uint64_t x;
    asm ("csrr %0, mip" : "=r" (x));
    return x;
}

static inline void w_mip(uint64_t x) {
    asm ("csrw mip, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The mie register is a 64-bit read/write register containing details about enabled interrupts.
 *
 * It has the following format:
 *
 * 63     11     10      9     8      7      6      5      4      3      2      1      0
 * +------+------+------+------+------+------+------+------+------+------+------+------+------+
 * | WPRI | MEIE | WPRI | SEIE | UEIE | MTIE | WPRI | STIE | UTIE | MSIE | WPRI | SSIE | USIE |
 * +------+------+------+------+------+------+------+------+------+------+------+------+------+
 *    52     1      1      1      1      1      1      1       1     1       1      1      1
 *
 * As with the mip register, each xE bit indicates an interrupt enable bit for a specific
 * interrupt type e.g. external, timer and software with privileges M, S and U.
 *
 * An interrupt will be handled (via the MTVEC ISR) if and only if it's pending bit is set (via the PLIC)
 * and it's interrupt enable bit is set.
 *
 */

#define MIE_MEIE_MASK   (1L << 11)
#define MIE_MEIE        (1L << 11)

#define MIE_SEIE_MASK   (1L << 9)
#define MIE_SEIE        (1L << 9)

#define MIE_UEIE_MASK   (1L << 8)
#define MIE_UEIE        (1L << 8)

#define MIE_MTIE_MASK   (1L << 7)
#define MIE_MTIE        (1L << 7)

#define MIE_STIE_MASK   (1L << 5)
#define MIE_STIE        (1L << 5)

#define MIE_UTIE_MASK   (1L << 4)
#define MIE_UTIE        (1L << 4)

#define MIE_MSIE_MASK   (1L << 3)
#define MIE_MSIE        (1L << 3)

#define MIE_SSIE_MASK   (1L << 1)
#define MIE_SSIE        (1L << 1)

#define MIE_USIE_MASK   (1L << 0)
#define MIE_USIE        (1L << 0)

static inline uint64_t r_mie() {
    uint64_t x;
    asm ("csrr %0, mie" : "=r" (x));
    return x;
}

static inline void w_mie(uint64_t x) {
    asm ("csrw mie, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The machine timer registers (mtime and mtimecmp) provide an interface with clock (or timer).
 * The mtime register is a 64-bit read only register that contains the current time.
 * The mtimecmp register is a 64-bit read/write register.
 *
 * The mtimecmp is a timer compare register, which causes a timer interrupt to be sent
 * when mtime contains a value greater or equal to the value of mtimecmp.
 *
 * The timer interrupt will only be handled if the MTIE bit is set in the mie register.
 *
 */

// These registers are accessed using memory mapped io (mmio). See clint.

/***********************************************************************************************************************
 * The mscratch, machine scratch register, is a 64-bit read/write register used store store a pointer to
 * some free space that a hart may use during a context switch.
 *
 */

static inline void w_mscratch(uint64_t x) {
    asm ("csrw mscratch, %0" : : "r" (x));
}

static inline uint64_t r_mscratch() {
    uint64_t x;
    asm ("csrr %0, mscratch" : "=r" (x));
    return x;
}

/***********************************************************************************************************************
 * The mepc, machine exception program counter, is a 64-bit read/write register used store the virtual (or physical)
 * address of the instruction that was executing when the exception was encountered.
 *
 */

static inline uint64_t r_mepc() {
    uint64_t x;
    asm ("csrr %0, mepc" : "=r" (x));
    return x;
}

static inline void w_mepc(uint64_t x) {
    asm ("csrw mepc, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The mcause, machine cause register, is a 64-bit read/write register that is used to store an indentifier
 * for the event that caused the trap e.g. page fault.
 *
 * The mcause register has the following format:
 *      63          62
 *      +-----------+----------------------+
 *      | INTERRUPT |    EXCEPTION CODE    |
 *      +-----------+----------------------+
 *            1                63
 *
 * The interrupt bit is used to indicate whether the interrupt is external (asynchronous) or internal (synchronous).
 * If an interrupt is synchronous, then it is said to be an exception.
 *
 * See table 3.6 for list of interrupts / exceptions in
 * https://content.riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf.
 *
 */

#define MCAUSE_INTERRUPT_MASK           (1L << 63)
#define MCAUSE_EXCEPTION_MASK           (~MCAUSE_INTERRUPT_MASK)

#define MCAUSE_INTERRUPT(mcause)        ((mcause & MCAUSE_INTERRUPT_MASK) >> 63)
#define MCAUSE_EXCEPTION(mcause)        (mcause & MCAUSE_EXCEPTION_MASK)

// Interrupts
#define CAUSE_USI  0
#define CAUSE_SSI  1
#define CAUSE_MSI  3

#define CAUSE_UTI  4
#define CAUSE_STI  5
#define CAUSE_MTI  6

#define CAUSE_UEI  7
#define CAUSE_SEI  9
#define CAUSE_MEI  11

// Exceptions
#define CAUSE_INST_ADDR_MISALIGNED          0
#define CAUSE_INST_ACCESS_FAULT             1

#define CAUSE_ILLEGAL_INST                  2

#define CAUSE_BREAKPOINT                    3

#define CAUSE_LOAD_ADDR_MISALIGNED          4
#define CAUSE_LOAD_ACCESS_FAULT             5

#define CAUSE_STORE_ADDR_MISALIGNED         6
#define CAUSE_STORE_ACCESS_FAULT            7

#define CAUSE_UECALL                        8
#define CAUSE_SECALL                        9
#define CAUSE_MECALL                        11

#define CAUSE_INST_PAGE_FAULT               12
#define CAUSE_LOAD_PAGE_FAULT               13
#define CAUSE_STORE_PAGE_FAULT              15

static inline uint64_t r_mcause() {
    uint64_t x;
    asm ("csrr %0, mcause" : "=r" (x));
    return x;
}

static inline void w_mcause(uint64_t x) {
    asm ("csrw mcause, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The mtval, machine trap value, is a 64-bit read/write register is used to store additional details
 * for a given trap.
 *
 * For example, for any instruction, load or store page fault, the mtval stores the faulting virtual (or physical)
 * address.
 *
 */

static inline uint64_t r_mtval() {
    uint64_t x;
    asm ("csrr %0, mtval" : "=r" (x));
    return x;
}

static inline void w_mtval(uint64_t x) {
    asm ("csrw mtval, %0" : : "r" (x));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SUPERVISOR LEVEL CSRS (Control and Status Registers)                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/***********************************************************************************************************************
 * The sstatus CSR is a 64-bit read and write register with the following format:
 *
 * 63   62     33         31     19    18    17     16        14        12
 * +----+------+----------+------+-----+-----+------+---------+---------+------+
 * | SD | WPRI | UXL[1:0] | WPRI | MXR | SUM | WPRI | XS[1:0] | FS[1:0] | WPRI | ...
 * +----+------+----------+------+-----+-----+------+---------+---------+------+
 *   1    29        2        12     1    1      1        2         2       4
 *
 *      12     8     7      5      4      3      1     0
 *      +------+-----+------+------+------+------+-----+-----+
 *  ... | WPRI | SPP | WPRI | SPIE | UPIE | WPRI | SIE | UIE |
 *      +------+-----+------+------+------+------+-----+-----+
 *         4      1     2      1      1      2     1      1
 *
 * The sstatus register offers a restricted view of the mstatus register. See mstatus (line 35).
 */

#define SSTATUS_SPP_MASK    (1L << 8)
#define SSTATUS_SPP_S       (1L << 8)
#define SSTATUS_SPP_U       (0L << 8)

#define SSTATUS_SPIE_MASK   (1L << 5)
#define SSTATUS_SPIE        (1L << 5)

#define SSTATUS_UPIE_MASK   (1L << 4)
#define SSTATUS_UPIE        (1L << 4)

#define SSTATUS_SIE_MASK    (1L << 1)
#define SSTATUS_SIE         (1L << 1)

#define SSTATUS_UIE_MASK    (1L << 0)
#define SSTATUS_UIE         (1L << 0)


static inline uint64_t r_sstatus() {
    uint64_t x;
    asm ("csrr %0, sstatus" : "=r" (x));
    return x;
}

static inline void w_sstatus(uint64_t x) {
    asm ("csrw sstatus, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The sepc is a 64-bit read/write register used store the virtual (or physical)
 * address of the instruction that was executing when the exception was encountered.
 *
 */

static inline uint64_t r_sepc() {
    uint64_t x;
    asm ("csrr %0, sepc" : "=r" (x));
    return x;
}

static inline void w_sepc(uint64_t x) {
    asm ("csrw sepc, %0" : : "r" (x));
}


/***********************************************************************************************************************
 * The stvec register is a 64-bit read/write register that holds the trap vector config
 * It consists of a base address for the trap vector (BASE) and a trap vector mode (mode)
 *
 * The stvec has the following format:
 *      63                   2
 *      +--------------------+------------+
 *      |        BASE        |    MODE    |
 *      +--------------------+------------+
 *               62                2
 *
 */

#define STVEC_MODE_MASK     (0x3)
#define STVEC_BASE_MASK     (~STVEC_MODE_MASK)

#define STVEC_MODE_DIRECT   0
#define STVEC_MODE_VECTORED 1

#define STVEC_BASE(mtvec)   (stvec & STVEC_BASE_MASK)
#define STVEC_MODE(mtvec)   (stvec & STVEC_MODE_MASK)

#define STVEC(base, mode)   ((base) | (mode & STVEC_MODE_MASK))

static inline uint64_t r_stvec() {
    uint64_t x;
    asm ("csrr %0, stvec" : "=r" (x));
    return x;
}

static inline void w_stvec(uint64_t x) {
    asm ("csrw stvec, %0" : : "r" (x));
}


/***********************************************************************************************************************
 * The sip register is a 64-bit read/write register containing details about pending interrupts.
 *
 * It has the following format:
 *
 * 63     9      8      7      5      4      3      1      0
 * +------+------+------+------+------+------+------+------+------+
 * | WPRI | SEIP | UEIP | WPRI | STIP | UTIP | WPRI | SSIP | USIP |
 * +------+------+------+------+------+------+------+------+------+
 *
 *
 * The sip register offers a restricted view of the mip register. See mip (line ??).
 */

#define SIP_SEIP_MASK   (1L << 9)
#define SIP_SEIP        (1L << 9)

#define SIP_UEIP_MASK   (1L << 8)
#define SIP_UEIP        (1L << 8)

#define SIP_STIP_MASK   (1L << 5)
#define SIP_STIP        (1L << 5)

#define SIP_UTIP_MASK   (1L << 4)
#define SIP_UTIP        (1L << 4)

#define SIP_SSIP_MASK   (1L << 1)
#define SIP_SSIP        (1L << 1)

#define SIP_USIP_MASK   (1L << 0)
#define SIP_USIP        (1L << 0)

static inline uint64_t r_sip() {
    uint64_t x;
    asm ("csrr %0, sip" : "=r" (x));
    return x;
}

static inline void w_sip(uint64_t x) {
    asm ("csrw sip, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The sie register is a 64-bit read/write register containing details about enabled interrupts.
 *
 * It has the following format:
 *
 * 63     9      8      7      5      4      3      1      0
 * +------+------+------+------+------+------+------+------+------+
 * | WPRI | SEIE | UEIE | WPRI | STIE | UTIE | WPRI | SSIE | USIE |
 * +------+------+------+------+------+------+------+------+------+
 *
 *
 * The sie register offers a restricted view of the mie register. See mie (line ??).
 */

#define SIE_SEIE_MASK   (1L << 9)
#define SIE_SEIE        (1L << 9)

#define SIE_UEIE_MASK   (1L << 8)
#define SIE_UEIE        (1L << 8)

#define SIE_STIE_MASK   (1L << 5)
#define SIE_STIE        (1L << 5)

#define SIE_UTIE_MASK   (1L << 4)
#define SIE_UTIE        (1L << 4)

#define SIE_SSIE_MASK   (1L << 1)
#define SIE_SSIE        (1L << 1)

#define SIE_USIE_MASK   (1L << 0)
#define SIE_USIE        (1L << 0)

static inline uint64_t r_sie() {
    uint64_t x;
    asm ("csrr %0, sie" : "=r" (x));
    return x;
}

static inline void w_sie(uint64_t x) {
    asm ("csrw sie, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The scause is a 64-bit read/write register that is used to store an indentifier
 * for the event that caused the trap e.g. page fault.
 *
 * The scause register has the following format:
 *      63          62
 *      +-----------+----------------------+
 *      | INTERRUPT |    EXCEPTION CODE    |
 *      +-----------+----------------------+
 *            1                63
 * It is identical to the mcause register.
 */

#define SCAUSE_INTERRUPT_MASK           (1L << 63)
#define SCAUSE_EXCEPTION_MASK           (~SCAUSE_INTERRUPT_MASK)

#define SCAUSE_INTERRUPT(scause)        ((scause & SCAUSE_INTERRUPT_MASK) >> 63)
#define SCAUSE_EXCEPTION(scause)        (scause & SCAUSE_EXCEPTION_MASK)


static inline uint64_t r_scause() {
    uint64_t x;
    asm ("csrr %0, scause" : "=r" (x));
    return x;
}

static inline void w_scause(uint64_t x) {
    asm ("csrw scause, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The stval is a 64-bit read/write register is used to store additional details
 * for a given trap.
 *
 * It is identical to the mtval register.
 */

static inline uint64_t r_stval() {
    uint64_t x;
    asm ("csrr %0, stval" : "=r" (x));
    return x;
}

static inline void w_stval(uint64_t x) {
    asm ("csrw stval, %0" : : "r" (x));
}

/***********************************************************************************************************************
 * The satp, supervisor address translation and protection, register is a 64-bit read/write register
 * which controls supervisor-mode address translation and protection. This register holds the
 * physical address of the root page table (PNN, physical page number), an address space identifier (ASID)
 * and the MODE field, which selects current address-translation scheme.
 *
 * The satp register has the following format:
 *      63     59          43
 *      +------+------------+-------------------+
 *      | MODE |    ASID    |        PPN        |
 *      +------+------------+-------------------+
 *         4         16              44
 *
 */

static void w_satp(uint64_t x) {
    asm ("csrw satp, %0" : : "r" (x));
}

static uint64_t r_satp() {
    uint64_t x;
    asm ("csrr %0, satp" : "=r" (x));
    return x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //TINY_OS_RISCV_H
