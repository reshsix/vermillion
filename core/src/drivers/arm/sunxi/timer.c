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

#include <_types.h>
#include <_utils.h>
#include <string.h>
#include <vermillion/drivers.h>
#include <vermillion/interrupts.h>

#define TMR_IRQ_EN(x)  *(volatile u32*)(x + 0x0)
#define TMR_IRQ_STA(x) *(volatile u32*)(x + 0x4)
#define TMR_CTRL(x, n) *(volatile u32*)(x + ((n + 1) * 0x10))
#define TMR_INTV(x, n) *(volatile u32*)(x + ((n + 1) * 0x10) + 0x4)
#define TMR_CUR(x, n)  *(volatile u32*)(x + ((n + 1) * 0x10) + 0x8)

struct timer
{
    u32 base;
    u8 id;
    u16 irq;

    union config config;
};

static struct timer timers[8] = {0};
static void ack(u32 base, u8 id){ TMR_IRQ_STA(base) |= 1 << id; }
static void ack0(void){ ack(timers[0].base, timers[0].id); }
static void ack1(void){ ack(timers[1].base, timers[1].id); }
static void ack2(void){ ack(timers[2].base, timers[2].id); }
static void ack3(void){ ack(timers[3].base, timers[3].id); }
static void ack4(void){ ack(timers[4].base, timers[4].id); }
static void ack5(void){ ack(timers[5].base, timers[5].id); }
static void ack6(void){ ack(timers[6].base, timers[6].id); }
static void ack7(void){ ack(timers[7].base, timers[7].id); }
static void (*acks[8]) = {ack0, ack1, ack2, ack3, ack4, ack5, ack6, ack7};

static void
init(void **ctx, u32 base, u8 id, u16 irq)
{
    struct timer *ret = NULL;

    int found = -1;
    for (u8 i = 0; i < sizeof(timers) / sizeof(struct timer); i++)
    {
        if (timers[i].base == 0)
            found = i;
    }

    if (found >= 0)
    {
        ret = &(timers[found]);

        ret->base = base;
        ret->id = id;
        ret->irq = irq;

        ret->config.timer.clock = 24000000;

        intr_config(ret->irq, acks[found], true, 0);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct timer *tmr = ctx;
    intr_config(tmr->irq, NULL, false, 0);
    tmr->base = 0;
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct timer *t = ctx;
    memcpy(cfg, &(t->config), sizeof(union config));
    return true;
}

static bool
block_write(void *ctx, u8 *buffer, u32 block)
{
    bool ret = (block == 0);

    if (ret)
    {
        struct timer *tmr = ctx;
        u32 data = 0;

        memcpy(&data, buffer, sizeof(u32));

        TMR_IRQ_EN(tmr->base) |= 1 << tmr->id;
        TMR_CTRL(tmr->base, tmr->id) &= ~0x1;

        TMR_INTV(tmr->base, tmr->id) = data;
        TMR_CTRL(tmr->base, tmr->id) = (1 << 7) | (1 << 2);

        TMR_CTRL(tmr->base, tmr->id) |= 0x2;
        while (TMR_CTRL(tmr->base, tmr->id) & 0x2);

        TMR_CTRL(tmr->base, tmr->id) |= 0x1;
        while (1)
        {
            intr_wait();
            u32 x = TMR_CUR(tmr->base, tmr->id);
            if (x == 0 || x > data)
                break;
        }

        TMR_CTRL(tmr->base, tmr->id) &= ~0x1;
        TMR_IRQ_EN(tmr->base) &= ~(1 << tmr->id);
    }

    return ret;
}

DECLARE_DRIVER(sunxi_timer)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_BLOCK,
    .type = DRIVER_TYPE_TIMER,
    .config.get = config_get,
    .interface.block.write = block_write
};
