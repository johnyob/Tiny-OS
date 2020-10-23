////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 7/11/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        interrupt.c
//      Environment: Tiny OS
//      Description: Interrupt implements methods for dealing with external interrupts,
//                   specifically using the PLIC (Platform Local Interrupt Controller).
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <lib/stdint.h>

#include <debug.h>
#include <riscv.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

#include <dev/uart.h>
#include <dev/plic.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PLIC (Platform Local Interrupt Controller)                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The PLIC is used to manager external interrupts. It connects the to EI (External Interrupt) pin of a hart.
// The EI pin is controlled using the MEIE (or SEIE) bit in the MIE (SIE) registers.
// When ever the EI pin is set (and the xEIE bit is set), we query the PLIC to determine the source of the
// external interrupt.
//
// For QEMU's virt riscv64 arch, the PLIC is controlled via MMIO registers:
// - Priority:      Sets the priority of a particular interrupt source.
// - Pending:       Contains a list of interrupts that are pending.
// - Enable:        Stores a bitmask of enabled/disabled interrupts.
// - Threshold:     Sets the threshold that interrupts must meet before being able to trigger.
// - Read:          Returns the next interrupt in the priority order.
// - Write:         Completes handling of a particular interrupt.
//
// The PLIC used by QEMU's virt riscv64 architecture is identical to the SiFive PLIC.
// See chapter 10 of https://sifive.cdn.prismic.io/sifive%2F834354f0-08e6-423c-bf1f-0cb58ef14061_fu540-c000-v1.0.pdf.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PLIC_START          (0xc000000L)
#define PLIC_SIZE           (0x4000000L)

/*
 * The source priority registers have the address space [0x0c000004, 0x0c0000D8].
 * The PLIC supports 7 levels of priority and each interrupt source can be assigned
 * a priority by writing to it's respective 32-bit word at index [irq] in the priority register.
 *
 * A priority of 0 => the source can never trigger an interrupt.
 * A priority of 1 is the lowest "active" priority and a priority of 7 is the highest.
 *
 */
#define PLIC_PRIORITY               (PLIC_START + 0x0)

/*
 * Interrupts can be filtered at hart level. We do this using enable bits.
 * Each hart has a bitmask, where interrupt [irq] is enabled iff bit [irq] is set.
 */
#define PLIC_SENABLE_BASE           (PLIC_START + 0x2080)
#define PLIC_SENABLE(id)            (PLIC_SENABLE_BASE + id * 0x100)

/*
 * Each hart has a interrupt priority register with is a 32-bit register
 * that stores a priority that all incoming interrupts to the hart must satisfy.
 * If the interrupt doesn't satisfy the threshold, then the hart is not notified.
 */
#define PLIC_SPRIORITY_TSH_BASE     (PLIC_START + 0x201000)
#define PLIC_SPRIORITY_TSH(id)      (PLIC_SPRIORITY_TSH_BASE + id * 0x2000)

/*
 * To handle interrupts, the PLIC uses a claim/complete process.
 * When an interrupt occurs, the PLIC notifies the hart. The hart
 * then claims the interrupt from the PLIC by reading the claim register.
 *
 * The claim register contains the irq (id) of the highest-priority pending interrupt
 * (or zero) if there is no pending interrupt.
 *
 * The hart can signal that it has completed handling the interrupt by
 * the irq of the interrupt into the claim register (called the complete register
 * in this context).
 */
#define PLIC_SCLAIM_BASE            (PLIC_START + 0x201004)
#define PLIC_SCLAIM(id)             (PLIC_SCLAIM_BASE + id * 0x2000)

enum {
    PLIC_UART0_IRQ = 10,
    PLIC_RTC_IRQ = 11,
    PLIC_VIRTIO_IRQ = 1, // 1 to 8
    PLIC_VIRTIO_COUNT = 8,
    PLIC_PCIE_IRQ = 0x20, // 32 to 35
    PLIC_VIRTIO_NDEV = 0x35 // Arbitrary maximum number of interrupts
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL PLIC METHODS                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Function:    plic_claim
 * -----------------------
 * The plic_claim function implements the claim step in the claim/complete interrupt
 * handling mechanism the PLIC uses. We simply claim an interrupt by reading the
 * claim register.
 *
 * @returns:    The interrupt source id (irq) of the highest-priority pending
 *              interrupt.
 *              Zero is returned if there is no pending interrupt.
 *
 */
static inline uint32_t plic_claim() {
    return *(uint32_t*)(PLIC_SCLAIM(r_hartid()));
}

/*
 * Procedure:   plic_complete
 * --------------------------
 * The plic_complete procedure implements the complete step in the claim/complete interrupt
 * handling mechanism the PLIC uses. We simply complete an interrupt with interrupt source id
 * [irq] by writing it into the claim register (referred to as the complete register in this context).
 *
 * @uint32_t irq:   The interrupt source id (irq) of the interrupt that we wish
 *                  complete.
 *
 */
static inline void plic_complete(uint32_t irq) {
    *(uint32_t*)(PLIC_SCLAIM(r_hartid())) = irq;
}

/*
 * Procedure:   plic_irq_enable
 * ----------------------------
 * This procedure sets the enable bit for the interrupt with source id [irq]
 * in the current hart's plic enable register.
 *
 * @uint32_t irq:   The interrupt source id (irq) of the interrupt that
 *                  we wish to enable locally.
 *
 */
static inline void plic_irq_enable(uint32_t irq) {
    *(uint32_t*)PLIC_SENABLE(r_hartid()) |= (1 << irq);
}

/*
 * Procedure:   plic_irq_priority
 * ------------------------------
 * This procedure sets the global priority of a given interrupt source with id [irq]
 * to have priority [priority].
 *
 * @uint32_t irq:       The interrupt source id (irq) of the interrupt source
 *                      whose global priority we wish to set.
 * @uint8_t priority:   The priority that we wish interrupt source with id [irq]
 *                      to have.
 *                      The priority must satisfy [0 <= priority <= 7].
 *
 */
static inline void plic_irq_priority(uint32_t irq, uint8_t priority) {
    assert(priority <= 7);
    ((uint32_t*)(PLIC_PRIORITY))[irq] = priority;
}

/*
 * Procedure:   plic_irq_threshold
 * -------------------------------
 * This procedure sets the local interrupt priority threshold of the current hart to
 * [threshold].
 *
 * @uint8_t threshold:  The priority threshold that interrupts must satisfy
 *                      for the current hart.
 *                      The threshold must satisfy [0 <= threshold <= 7].
 *
 */
static inline void plic_irq_threshold(uint8_t threshold) {
    assert(threshold <= 7);
    *(uint32_t*)PLIC_SPRIORITY_TSH(r_hartid()) = (uint32_t)threshold;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNAL PLIC METHODS                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Procedure:   plic_handle_interrupt
 * ----------------------------------
 * The plic_handle_interrupt procedure handles any external interrupts from the PLIC.
 * The procedure first performs the PLIC's claim process, yielding the interrupt source id [irq].
 * We must assert that [irq != 0], since the procedure [plic_handle_interrupt] must be called when
 * a plic interrupt occurs. We then dispatch the interrupt handling to the correct handler
 * and then perform the PLIC's complete procedure, notifying the PLIC that we have finished
 * handling the particular interrupt.
 *
 */
void plic_handle_interrupt(trap_frame_t* tf) {
    uint32_t irq = plic_claim();
    assert(irq != 0);

    switch (irq) {
        case PLIC_UART0_IRQ:
            uart_handle_interrupt(tf);
            break;
        default:
            panic("Unhandled external interrupt. Hart: %d, interrupt: %d.\n", r_hartid(), irq);
            break;
    }

    plic_complete(irq);
}

/*
 * Procedure:   plic_init
 * ----------------------
 * Initializes the PLIC with a global (non-hart-local) configuration.
 * In the context of the PLIC, the only global configuration is the priority of
 * interrupt sources.
 *
 */
void plic_init() {
    plic_irq_priority(PLIC_UART0_IRQ, 1);
}

void plic_vm_init() {
    kmap(PLIC_START, PLIC_START, PLIC_SIZE, PTE_R | PTE_W);
    info("plic: \t%#p -> %#p\n", PLIC_START, PLIC_START + PLIC_SIZE);
}

/*
 * Procedure:   plic_hart_init
 * ---------------------------
 * Initializes the PLIC with a local (hart) configuration.
 * This simply consists of enabling the local interrupt sources and
 * setting the local threshold.
 *
 */
void plic_hart_init() {
    plic_irq_enable(PLIC_UART0_IRQ);
    plic_irq_threshold(0);
}


