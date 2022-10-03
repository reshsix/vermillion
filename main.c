/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include "common/types.h"
#include "common/interrupts.h"

#define GPIO 0x01C20800
#define PA_CFG1 *(volatile u32*)(GPIO + 0x4)
#define PA_DAT  *(volatile u32*)(GPIO + 0x10)

#define TIMER 0x01C20C00
#define TMR_IRQ_EN  *(volatile u32*)(TIMER + 0x0)
#define TMR_IRQ_STA *(volatile u32*)(TIMER + 0x4)
#define TMR0_CTRL  *(volatile u32*)(TIMER + 0x10)
#define TMR0_INTV  *(volatile u32*)(TIMER + 0x14)
#define TMR0_CUR   *(volatile u32*)(TIMER + 0x18)
#define IRQ_TIMER0 50

static volatile u8 count = 0;

static interrupt(irq)
irq_handler(void)
{
    /* Get interrupt information */
    u8 core = 0;
    u16 number = intr_irq_info(&core);

    if (number == IRQ_TIMER0)
    {
        /* Turn led on and off */
        if (count % 2)
            PA_DAT = 0;
        else
            PA_DAT = 1 << 15;
        count++;

        /* Acknowledge interrupt on timer */
        TMR_IRQ_STA = 0x1;
    }

    /* Acknowledge interrupt on GIC */
    intr_irq_ack(core, number);
}

extern void
kernel_main(void)
{
    /* Enables board's red led */
    PA_CFG1 = 0x17777777;

    /* Configure interrupts */
    ivt[IVT_IRQ] = (u32)irq_handler;

    arm_disable_irq();

    gic_disable_dist();
    gic_disable();
    gic_priority(0xFF);

    gic_intr_target(IRQ_TIMER0, -1);
    gic_intr_activity(IRQ_TIMER0, false);
    gic_intr_priority(IRQ_TIMER0, 0);
    gic_intr_sensitivity(IRQ_TIMER0, true, false);
    gic_intr_activity(IRQ_TIMER0, true);
    gic_intr_target(IRQ_TIMER0, 0);

    gic_enable();
    gic_enable_dist();

    arm_enable_irq();

    /* Configure timer */
    TMR_IRQ_EN = 0x1;
    TMR0_INTV = 0x8000 / 4;
    TMR0_CTRL = 0x0;
    TMR0_CTRL |= 0x2;
    TMR0_CTRL |= 0x1;

    /* Wait for 10 interruptions */
    count = 0; /* TODO clear bss on boot! */
    while (count < 10)
        arm_wait_interrupts();

    /* Disable timer */
    TMR0_CTRL = 0x0;

    /* Loop forever */
    while (1)
        arm_wait_interrupts();
}
