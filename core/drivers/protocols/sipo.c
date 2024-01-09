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

#include <core/gpio.h>
#include <core/stream.h>

struct sipo
{
    dev_gpio *gpio;
    u16 data;
    u16 clock;
    u16 latch;
    bool lsbfirst;
};

static void
init(void **ctx, dev_gpio *gpio, u16 data, u16 clock, u16 latch, bool lsbfirst)
{
    struct sipo *ret = mem_new(sizeof(struct sipo));

    if (ret)
    {
        ret->gpio = gpio;
        ret->data = data;
        ret->clock = clock;
        ret->latch = latch;
        ret->lsbfirst = lsbfirst;

        gpio_config(gpio, data, GPIO_OUT, GPIO_PULLOFF);
        gpio_config(gpio, clock, GPIO_OUT, GPIO_PULLOFF);
        gpio_config(gpio, latch, GPIO_OUT, GPIO_PULLOFF);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    if (ctx)
    {
        struct sipo *s = ctx;
        gpio_config(s->gpio, s->data, GPIO_OFF, GPIO_PULLOFF);
        gpio_config(s->gpio, s->clock, GPIO_OFF, GPIO_PULLOFF);
        gpio_config(s->gpio, s->latch, GPIO_OFF, GPIO_PULLOFF);
    }

    mem_del(ctx);
}

static bool
write(void *ctx, u32 idx, void *data)
{
    bool ret = (idx == 0);

    if (ret)
    {
        struct sipo *s = ctx;
        gpio_set(s->gpio, s->latch, false);
        gpio_set(s->gpio, s->data, false);

        for (u8 i = 0; i < 8; i++)
        {
            gpio_set(s->gpio, s->clock, false);
            if (s->lsbfirst)
                gpio_set(s->gpio, s->data, *((u8*)data) & (1 << (7 - i)));
            else
                gpio_set(s->gpio, s->data, *((u8*)data) & (1 << i));
            gpio_set(s->gpio, s->clock, true);
        }

        gpio_set(s->gpio, s->latch, true);
    }

    return ret;
}

drv_decl (stream, sipo)
{
    .init = init, .clean = clean,
    .write = write
};
