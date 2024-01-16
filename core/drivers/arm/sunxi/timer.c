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

#include <core/types.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/mem.h>

#include <core/pic.h>
#include <core/timer.h>
#include <core/thread.h>

#define TMR_IRQ_EN(x)  *(volatile u32*)(x + 0x0)
#define TMR_IRQ_STA(x) *(volatile u32*)(x + 0x4)
#define TMR_CTRL(x, n) *(volatile u32*)(x + ((n + 1) * 0x10))
#define TMR_INTV(x, n) *(volatile u32*)(x + ((n + 1) * 0x10) + 0x4)
#define TMR_CUR(x, n)  *(volatile u32*)(x + ((n + 1) * 0x10) + 0x8)

struct timer
{
    u32 base;
    u8 id;

    dev_pic *pic;
    u16 irq;

    struct timer_cb cb;
};

static void
handler(void *arg)
{
    struct timer *tmr = arg;
    TMR_IRQ_STA(tmr->base) |= 1 << tmr->id;

    tmr->cb.handler(tmr->cb.arg);
}

static void
init(void **ctx, u32 base, u8 id, dev_pic *pic, u16 irq)
{
    struct timer *ret = mem_new(sizeof(struct timer));

    if (ret)
    {
        ret->base = base;
        ret->id = id;
        ret->pic = pic;
        ret->irq = irq;

        TMR_INTV(base, id) = 0xFFFFFFFF;
        TMR_CTRL(base, id) = 1 << 2;
        TMR_IRQ_STA(base) |= 1 << id;
        TMR_IRQ_EN(base) |= 1 << id;

        pic_config(pic, ret->irq, true, handler, ret, PIC_EDGE_L);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct timer *tmr = ctx;
    pic_config(tmr->pic, tmr->irq, false, NULL, NULL, PIC_EDGE_L);
    tmr->base = 0;
    mem_del(ctx);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = true;

    (void)ctx;
    switch (idx)
    {
        case 0:
            *width = sizeof(struct timer_cb);
            *length = 1;
            break;
        case 1:
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
        case 0:
            ret = (block == 0);

            if (ret)
                mem_copy(buffer, &(tmr->cb), sizeof(struct timer_cb));
            break;

        case 1:
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
        case 0:
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

        case 1:
            ret = (block == 0);

            if (ret)
                pic_wait(tmr->pic);
            break;
    }

    return ret;
}

drv_decl (timer, sunxi_timer)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
