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

#include <i686/env.h>

#include <core/types.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/mem.h>

#include <core/pic.h>
#include <core/timer.h>
#include <core/thread.h>

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_CONTROL  0x43

struct timer
{
    dev_pic *pic;
    u16 irq;

    struct timer_cb cb;

    u8 counter;
};
struct timer *tmr = NULL;

static void
handler(void *arg)
{
    struct timer *tmr = arg;
    tmr->cb.handler(tmr->cb.arg);
}

static void
init(void **ctx, dev_pic *pic, u16 irq)
{
    if (!tmr)
    {
        tmr = mem_new(sizeof(struct timer));
        if (tmr)
        {
            tmr->pic = pic;
            tmr->irq = irq;

            *ctx = tmr;
        }
    }
}

static void
clean(void *ctx)
{
    if (ctx == tmr)
    {
        pic_config(tmr->pic, tmr->irq, false, NULL, NULL, PIC_EDGE_H);
        mem_del(tmr);
    }
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = true;

    if (ctx)
    {
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
    }
    else
        ret = false;

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = true;

    struct timer *tmr = ctx;
    if (ctx)
    {
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
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = true;

    struct timer *tmr = ctx;
    if (ctx)
    {
        switch (idx)
        {
            case 0:
                ret = (block == 0);

                struct timer_cb cb = {0};
                if (ret)
                {
                    mem_copy(&cb, buffer, sizeof(struct timer_cb));
                    ret = (cb.delay < ((65535 / 119) * 100));
                }

                if (ret)
                {
                    mem_copy(&(tmr->cb), buffer, sizeof(struct timer_cb));
                    pic_config(tmr->pic, tmr->irq, tmr->cb.enabled,
                               handler, tmr, PIC_EDGE_H);

                    if (tmr->cb.enabled)
                        /* Channel 0, Lobyte/hibyte, rate generator */
                        out8(PIT_CONTROL, (3 << 4) | (2 << 1));
                    else
                        /* Channel 0, Lobyte/hibyte,
                           One-shot with inexistent gate */
                        out8(PIT_CONTROL, (3 << 4) | (2 << 1));

                    u16 reload = (tmr->cb.delay * 119) / 100;
                    out8(PIT_CHANNEL0, reload & 0xFF);
                    out8(PIT_CHANNEL0, reload >> 8);
                }
                break;

            case 1:
                ret = (block == 0);

                if (ret)
                    pic_wait(tmr->pic);
                break;
        }
    }

    return ret;
}

drv_decl (timer, i686_timer)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
