#include "idt.h"

#include "pic.h"

#include <stdbool.h>
#include <stdint.h>

#define GATE_MASK 0b00001111
#define PRIVILEGE_MASK 0b01100000
#define PRESENT_MASK 0b10000000

// interrupt gates disable interrupts on entry, trap do not.
#define GATE_INTR 0b1110
#define GATE_TRAP 0b1111

// privilege level of gates
#define PRIV_KERNEL 0b0000000
#define PRIV_KDEVICE 0b0100000
#define PRIV_UDEVICE 0b1000000
#define PRIV_USER 0b1100000

#define GATE_PRESENT 0b10000000

#define DEFAULT_FLAGS (GATE_PRESENT | PRIV_KERNEL | GATE_INTR)

// see GDT for details
#define CODE_SEGMENT 0x8

uint32_t getOffset(IdtEntry entry) {
    // reconstruct the address
    return (((uint32_t)(entry.high) << 16) | (uint32_t)(entry.low));
}

uint8_t getGateType(IdtEntry entry) { return entry.flags & GATE_MASK; }

uint8_t getPrivilegeLevels(IdtEntry entry) {
    // mask the privilege bits then shift them to be 0 based
    return (entry.flags & PRIVILEGE_MASK) >> 5;
}

bool isValid(IdtEntry entry) { return ((entry.flags & PRESENT_MASK) != 0); }

extern void *idt_stub_table[];
static IdtEntry idt[IDT_ENTRIES];
static idtr_t idtr;

void makeInterruptTable() {

    idtr.base = (uint32_t)(&idt);

    // used to calculate the end of the IDT, idtr.base + idtr.limit
    // this result is the address of the last byte of the IDT. Without -1 it
    // would be the byte after the IDT
    idtr.limit = sizeof(idt) - 1;

    // Setup all the idt descriptors for both ISRs and IRQs
    for (uint8_t idx = 0; idx < ISR_COUNT + IRQ_COUNT; idx++) {
        // note that IRQs are also interrupt gates, not trap.
        idtSetDesc(idx, idt_stub_table[idx], DEFAULT_FLAGS);
    }

    // we are offsetting IRQs to be right after the ISRs
    initPIC(ISR_COUNT);

    // load the IDT and then enable interrupts
    __asm__ volatile("lidt [%0]" : : "r"(&idtr));
    enableInterrupts();
}

void idtSetDesc(uint8_t idx, void *isr, uint8_t flags) {
    IdtEntry *desc = &idt[idx];

    // set offset
    desc->low = (uint32_t)isr & 0xFFFF; // lowest 2 bytes of the address
    desc->high =
        ((uint32_t)(isr) >> 16) & 0xFFFF; // upper 2 bytes of the address

    // set segment selector
    desc->selector = CODE_SEGMENT;

    desc->zero = 0;

    // set flags
    desc->flags = flags;
}

static int_handler_t isrHandlers[ISR_COUNT] = {};
static int_handler_t irqHandlers[IRQ_COUNT] = {};

void isrSetHandler(uint8_t isr_vec, int_handler_t handler) {
    isrHandlers[isr_vec] = handler;
}

void irqSetHandler(uint8_t irq_vec, int_handler_t handler) {
    irqHandlers[irq_vec] = handler;
}

void isrHandler(isr_registers_t *state) {

    int_handler_t handler = isrHandlers[state->vec_idx];
    if (handler != 0) {
        handler(state);
    }
}

void irqHandler(isr_registers_t *state) {

    int_handler_t handler = irqHandlers[state->vec_idx];
    if (handler != 0) {
        handler(state);
    }

    // very important or else we get no more IRQs.
    ackPIC(state->vec_idx);
}

#define INTERRUPT_MASK 0x0200

InterruptState getInterrupts(void) {
    uint32_t flags;

    // push flags register then pop into `flags`
    __asm__ volatile("pushf;"
                     "pop %0"
                     : "=g"(flags));

    if (flags & INTERRUPT_MASK) {
        return InterruptOn;
    }
    return InterruptOff;
}

InterruptState disableInterrupts(void) {
    InterruptState prev = getInterrupts();
    __asm__ volatile("cli");
    return prev;
}

InterruptState enableInterrupts(void) {
    InterruptState prev = getInterrupts();
    __asm__ volatile("sti");
    return prev;
}

InterruptState setInterrupts(InterruptState state) {
    switch (state) {
    case InterruptOff:
        return disableInterrupts();
    case InterruptOn:
        return enableInterrupts();
    default:
        return getInterrupts();
    }
}