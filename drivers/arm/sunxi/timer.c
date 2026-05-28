/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <arch/gic.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/timer.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

#define TMR_IRQ_EN(x)  *(volatile uint32_t*)(x + 0x0)
#define TMR_IRQ_STA(x) *(volatile uint32_t*)(x + 0x4)
#define TMR_CTRL(x, n) *(volatile uint32_t*)(x + ((n + 1) * 0x10))
#define TMR_INTV(x, n) *(volatile uint32_t*)(x + ((n + 1) * 0x10) + 0x4)
#define TMR_CUR(x, n)  *(volatile uint32_t*)(x + ((n + 1) * 0x10) + 0x8)

/* Driver definition */

struct timer
{
    uint32_t base;
    uint8_t id;

    uint8_t irq;
    void (*handler)(void *), *arg;
};

struct timer timers[2] = {0};

static void
callback(void *arg)
{
    struct timer *tmr = arg;
    TMR_IRQ_STA(tmr->base) |= 1 << tmr->id;

    if (tmr->handler)
        tmr->handler(tmr->arg);
}

static bool
alarm(void *ctx, uint32_t us, bool repeat, void (*handler)(void *), void *arg)
{
    struct timer *tmr = ctx;

    TMR_CTRL(tmr->base, tmr->id) &= ~(1 << 0);
    TMR_IRQ_EN(tmr->base) &= ~(1 << tmr->id);

    TMR_INTV(tmr->base, tmr->id) = 24 * us;
    TMR_CUR(tmr->base, tmr->id) = 0;

    gic_config(tmr->irq, (handler) ? callback : NULL, tmr, true, false);
    tmr->handler = handler;
    tmr->arg     = arg;

    if (us && handler)
    {
        TMR_IRQ_EN(tmr->base) |= 1 << tmr->id;
        TMR_CTRL(tmr->base, tmr->id) =
            (!repeat) << 7 | 1 << 2 | 1 << 1 | 1 << 0;
    }

    return true;
}

static void
wait(void *ctx)
{
    (void)ctx;
    gic_wait();
}

static const drv_timer sunxi_timer =
{
    .alarm = alarm, .wait = wait
};

/* Device creation */

extern dev_timer
sunxi_timer_init(uint8_t id)
{
    struct timer *ret = NULL;

    if (id < 2)
    {
        ret = &(timers[id]);

        ret->base = 0x01c20c00;
        ret->id = id;

        ret->irq = (id == 0) ? 50 : 51;
    }

    return (dev_timer){.driver = &sunxi_timer, .context = ret};
}

extern void
sunxi_timer_clean(dev_timer *t)
{
    if (t)
    {
        struct timer *tmr = t->context;

        TMR_CTRL(tmr->base, tmr->id) &= ~(1 << 0);
        TMR_IRQ_EN(tmr->base)  &= ~(1 << tmr->id);

        gic_config(tmr->irq, NULL, NULL, true, false);
        t->context = NULL;
    }
}
