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

#include <general/types.h>
#include <general/mem.h>

#include <hal/classes/pic.h>
#include <hal/classes/timer.h>

#define TMR_IRQ_EN(x)  *(volatile u32*)(x + 0x0)
#define TMR_IRQ_STA(x) *(volatile u32*)(x + 0x4)
#define TMR_CTRL(x, n) *(volatile u32*)(x + ((n + 1) * 0x10))
#define TMR_INTV(x, n) *(volatile u32*)(x + ((n + 1) * 0x10) + 0x4)
#define TMR_CUR(x, n)  *(volatile u32*)(x + ((n + 1) * 0x10) + 0x8)

/* Driver definition */

struct timer
{
    u32 base;
    u8 id;

    dev_pic *pic;
    u16 irq;

    struct timer_cb cb;
};

struct timer timers[2] = {0};

static void
handler(void *arg)
{
    struct timer *tmr = arg;
    TMR_IRQ_STA(tmr->base) |= 1 << tmr->id;

    tmr->cb.handler(tmr->cb.arg);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = true;

    (void)ctx;
    switch (idx)
    {
        case TIMER_CONFIG:
            *width = sizeof(struct timer_cb);
            *length = 1;
            break;
        case TIMER_WAIT:
            *width = 0;
            *length = 1;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = true;

    struct timer *tmr = ctx;
    switch (idx)
    {
        case TIMER_CONFIG:
            ret = (block == 0);

            if (ret)
                mem_copy(buffer, &(tmr->cb), sizeof(struct timer_cb));
            break;

        case TIMER_WAIT:
            ret = (block == 0);

            if (ret)
                pic_wait(tmr->pic);
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = true;

    struct timer *tmr = ctx;
    switch (idx)
    {
        case TIMER_CONFIG:
            ret = (block == 0);

            if (ret)
            {
                mem_copy(&(tmr->cb), buffer, sizeof(struct timer_cb));

                if (tmr->cb.enabled)
                {
                    TMR_INTV(tmr->base, tmr->id) = 24 * tmr->cb.delay;
                    TMR_CTRL(tmr->base, tmr->id) |= 1 << 1 | 1 << 0;
                }
                else
                    TMR_CTRL(tmr->base, tmr->id) &= ~(1 << 0);
            }
            break;

        case TIMER_WAIT:
            ret = (block == 0);

            if (ret)
                pic_wait(tmr->pic);
            break;
    }

    return ret;
}

static const drv_timer sunxi_timer =
{
    .stat = stat, .read = read, .write = write
};

/* Device creation */

extern dev_timer
sunxi_timer_init(u8 id, dev_pic *pic, u16 irq)
{
    struct timer *ret = NULL;

    if (id < 2)
    {
        ret = &(timers[id]);

        ret->base = 0x01c20c00;
        ret->id = id;
        ret->pic = pic;
        ret->irq = irq;

        TMR_INTV(ret->base, id) = 0xFFFFFFFF;
        TMR_CTRL(ret->base, id) = 1 << 2;
        TMR_IRQ_STA(ret->base) |= 1 << id;
        TMR_IRQ_EN(ret->base)  |= 1 << id;

        pic_config(pic, ret->irq, true, handler, ret, PIC_EDGE_L);
    }

    return (dev_timer){.driver = &sunxi_timer, .context = ret};
}

extern void
sunxi_timer_clean(dev_timer *t)
{
    if (t)
    {
        struct timer *tmr = t->context;

        pic_config(tmr->pic, tmr->irq, false, NULL, NULL, PIC_EDGE_L);
        tmr->base = 0;

        t->context = NULL;
    }
}
