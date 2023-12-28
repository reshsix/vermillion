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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>

struct sipo
{
    struct device *gpio;
    u16 data;
    u16 clock;
    u16 latch;
    bool lsbfirst;
};

static void
init(void **ctx, struct device *gpio, u16 data, u16 clock,
                 u16 latch, bool lsbfirst)
{
    struct sipo *ret = mem_new(sizeof(struct sipo));

    if (ret)
    {
        ret->gpio = gpio;
        ret->data = data;
        ret->clock = clock;
        ret->latch = latch;
        ret->lsbfirst = lsbfirst;

        pin_cfg(gpio, data, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
        pin_cfg(gpio, clock, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
        pin_cfg(gpio, latch, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    if (ctx)
    {
        struct sipo *s = ctx;
        pin_cfg(s->gpio, s->data, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
        pin_cfg(s->gpio, s->clock, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
        pin_cfg(s->gpio, s->latch, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
    }

    mem_del(ctx);
}

static bool
stream_write(void *ctx, u32 idx, u8 *data)
{
    bool ret = (idx == 0);

    if (ret)
    {
        struct sipo *s = ctx;
        pin_set(s->gpio, s->latch, false);
        pin_set(s->gpio, s->data, false);

        for (u8 i = 0; i < 8; i++)
        {
            pin_set(s->gpio, s->clock, false);
            if (s->lsbfirst)
                pin_set(s->gpio, s->data, data[0] & (1 << (7 - i)));
            else
                pin_set(s->gpio, s->data, data[0] & (1 << i));
            pin_set(s->gpio, s->clock, true);
        }

        pin_set(s->gpio, s->latch, true);
    }

    return ret;
}

DECLARE_DRIVER(sipo)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_STREAM,
    .type = DRIVER_TYPE_GENERIC,
    .stream.write = stream_write
};
