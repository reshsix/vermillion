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

    union config config;
};

static void
handler(void *arg)
{
    struct timer *t = arg;
    TMR_IRQ_STA(t->base) |= 1 << t->id;
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

        ret->config.timer.clock = 24000000;

        TMR_INTV(base, id) = 0xFFFFFFFF;
        TMR_CTRL(base, id) = 1 << 2 | 1 << 7;
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
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct timer *t = ctx;
    mem_copy(cfg, &(t->config), sizeof(union config));
    return true;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = (idx == 0 && block == 0);

    if (ret)
    {
        struct timer *tmr = ctx;

        u32 data = 0;
        mem_copy(&data, buffer, sizeof(u32));

        TMR_INTV(tmr->base, tmr->id) = data;
        TMR_CTRL(tmr->base, tmr->id) |= 1 << 1 | 1 << 0;
        while (1)
        {
            pic_wait(tmr->pic);
            u32 x = TMR_CUR(tmr->base, tmr->id);
            if (x == 0 || x > data)
                break;
        }
        TMR_CTRL(tmr->base, tmr->id) &= ~(1 << 0);
    }

    return ret;
}

drv_decl (timer, sunxi_timer)
{
    .init = init, .clean = clean,
    .config.get = config_get,
    .write = write
};
