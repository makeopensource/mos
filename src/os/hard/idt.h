#ifndef IDT_H
#define IDT_H

#include <stdbool.h>
#include <stdint.h>

#define IDT_ENTRIES 256
#define ISR_COUNT 32
#define IRQ_COUNT 16

typedef struct {
    uint16_t low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t high;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

typedef struct {
    uint32_t edi, esi, ebp, useless, ebx, edx, ecx, eax; // pushed by pushad
    uint32_t gs, fs, es, ds; // pushed segment selectors
    uint32_t vec_idx,
        err_code; // isr index and error code (0 when error is not applicable)
    uint32_t eip, cs, eflags, esp, ss; // pushed by CPU during interrupt
} isr_registers_t;

// upper half of upper (16-31), lower half of lower (0-15)
uint32_t getOffset(IdtEntry entry);         // addr of entry point of ISR
uint8_t getGateType(IdtEntry entry);        // bits 0-3 of flags
uint8_t getPrivilegeLevels(IdtEntry entry); // bits 5-6 of flags
bool isValid(IdtEntry entry);               // check present bit (flags 7th bit)

void makeInterruptTable();
void idtSetDesc(uint8_t idx, void *isr, uint8_t flags);

typedef void (*int_handler_t)(isr_registers_t *);

// sets handlers for ISRs and IRQs, 0 <= isr_vec < 32, 0 <= irq_vec < 16.
// note that IRQ0 is actually interrupt 32 (see init_pic)
void isrSetHandler(uint8_t isr_vec, int_handler_t handler);
void irqSetHandler(uint8_t irq_vec, int_handler_t handler);

typedef enum {
    InterruptOn,
    InterruptOff,
} InterruptState;

// get current state of interrupts
InterruptState getInterrupts(void);

// returns the previous state
InterruptState disableInterrupts(void);
InterruptState enableInterrupts(void);
InterruptState setInterrupts(InterruptState state);

#endif