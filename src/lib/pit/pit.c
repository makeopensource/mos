#include "pit.h"

#include "../../os/hard/idt.h"
#include "../../os/hard/port_io.h"

#include <stdint.h>

uint32_t timer_ticks = 0;

uint32_t get_ticks() { return timer_ticks; }

static void timer_handler(isr_registers_t *regs) {
    /* Increment 'Tick count' */
    timer_ticks++;
}

void init_pit() {
    InterruptState iprev = disableInterrupts();
    irqSetHandler(0, timer_handler);
    init_timer(
        100); // 100 hz is just a good general frequency according to the wiki
    setInterrupts(iprev);
}

void init_timer(int hz) {
    int divisor = OSCILLATIONS / hz; /* divisor */
    uint8_t channel = 0b00000000;
    uint8_t access_mode = 0b00110000;
    uint8_t op_mode = 0b00000110;
    uint8_t bcd_mode = 0b00000000;
    outb(CMD_REG_PORT,
         channel | access_mode | op_mode |
             bcd_mode); /* Write 0b00110110 to the command register */
    outb(CHAN_0_PORT, divisor & 0xFF); /* Set low byte of divisor */
    outb(CHAN_0_PORT, divisor >> 8);   /* Set high byte of divisor */
}

void sleep(uint32_t dur) {
    uint32_t start = timer_ticks;

    /* Timer ticks increases continuosly */
    while (timer_ticks - start < dur) {
        __asm__("hlt");
    }
}
