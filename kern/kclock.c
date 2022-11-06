/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/trap.h>
#include <kern/picirq.h>

/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 *
 * Why it is necessary?
 */

/*
 * When programming the RTC, it is extremely imperative 
 * that the NMI (non-maskable-interrupt) and other      
 * interrupts are disabled. This is because if an       
 * interrupt happens, the RTC may be left in an         
 * "undefined" (non functional) state. This would       
 * usually not be too big a deal, except for two things.
 * 
 * The RTC is never initialized by BIOS, and it is      
 * backed up with a battery. So even a cold reboot may  
 * not be enough to get the RTC out of an undefined state! 
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */
    // LAB 4: Your code here

    uint8_t res = 0;

    nmi_disable();

    outb(CMOS_CMD, reg);
    res = inb(CMOS_DATA);

    nmi_enable();
    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here

    nmi_disable();

    outb(CMOS_CMD, reg);
    outb(CMOS_DATA, value);

    nmi_enable();
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

void
rtc_timer_pic_interrupt(void) {
    // LAB 4: Your code here
    // Enable PIC interrupts.

    pic_irq_unmask(IRQ_CLOCK);
}

void
rtc_timer_pic_handle(void) {
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);
}

/*
 * frequency =  32768 >> (rate-1);
 * rate = lower 4 bits of A register
 */

void
rtc_timer_init(void) {
    // LAB 4: Your code here
    // (use cmos_read8/cmos_write8)

    uint8_t a_reg = cmos_read8(0xA);
    a_reg |= 0xF;
    cmos_write8(0xA, a_reg);

    uint8_t b_reg = cmos_read8(0xB);
    b_reg |= RTC_PIE;
    cmos_write8(0xB, b_reg);
}

/*
 * It is important to know that upon a IRQ 8, 
 * Status Register C will contain a bitmask 
 * telling which interrupt happened. The RTC 
 * is capable of producing a periodic interrupt 
 * (what this article describes), an update 
 * ended interrupt, and an alarm interrupt. 
 * If you are only using the RTC as a simple 
 * timer this is not important. What is important 
 * is that if register C is not read after an IRQ 8,
 * then the interrupt will not happen again. So, 
 * even if you don't care about what type of 
 * interrupt it is, just attach this code to the 
 * bottom of your IRQ handler to be sure you get 
 * another interrupt.
 */

uint8_t
rtc_check_status(void) {
    // LAB 4: Your code here
    // (use cmos_read8)

    return cmos_read8(0xC);
}
