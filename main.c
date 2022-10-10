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

#include "common/ports.h"
#include "common/timers.h"
#include "common/interrupts.h"

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

static volatile u8 count = 0;

static interrupt(irq)
irq_handler(void)
{
    /* Get interrupt information */
    enum intr_core c = 0;
    enum intr_number n = intr_irq_info(&c);

    if (n == IRQ_TIMER0)
    {
        /* Turn led on and off */
        pin_write(PA15, !(count & 0x1));
        count++;

        /* Acknowledge interrupt on timer */
        timer_ack(TIMER0);
    }

    /* Acknowledge interrupt on GIC */
    intr_irq_ack(c, n);
}

extern void
kernel_main(void)
{
    /* Enables R_PIO on the power controller */
    APB0_GATE = 1;

    /* Enables and turns green led on */
    pin_config(PL10, PIN_CFG_OUT);
    pin_write(PL10, true);

    /* Enables board's red led */
    pin_config(PA15, PIN_CFG_OUT);

    /* Configure interrupts */
    ivt[IVT_IRQ] = irq_handler;

    arm_disable_irq();

    gic_disable_dist();
    gic_disable();
    gic_priority(0xFF);

    gic_intr_target(IRQ_TIMER0, INTR_CORE_NONE);
    gic_intr_activity(IRQ_TIMER0, false);
    gic_intr_priority(IRQ_TIMER0, 0);
    gic_intr_sensitivity(IRQ_TIMER0, true, false);
    gic_intr_activity(IRQ_TIMER0, true);
    gic_intr_target(IRQ_TIMER0, INTR_CORE0);

    gic_enable();
    gic_enable_dist();

    arm_enable_irq();

    /* Configure timer */
    timer_enable(TIMER0);
    timer_interval_set(TIMER0, 0x8000 / 4);
    timer_config(TIMER0, false, TIMER_CLK_32KHZ);
    timer_reload(TIMER0);
    timer_start(TIMER0);

    /* Wait for 10 interruptions */
    while (count < 10)
        arm_wait_interrupts();

    /* Disable timer */
    timer_stop(TIMER0);

    /* Loop forever */
    while (1)
        arm_wait_interrupts();
}
