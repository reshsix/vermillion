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
#include <core/utils.h>
#include <core/drivers.h>

#include <core/mem.h>

struct buzzer
{
    dev_timer *timer;
    dev_gpio *gpio;
    u16 pin;

    union config config;
};

static void
init(void **ctx, dev_gpio *gpio, u16 pin, dev_timer *timer)
{
    struct buzzer *ret = NULL;

    if (timer && gpio)
        ret = mem_new(sizeof(struct buzzer));

    if (ret)
    {
        ret->timer = timer;
        ret->gpio = gpio;
        ret->pin = pin;

        ret->config.audio.freq = 48000;
        ret->config.audio.format = DRIVER_AUDIO_FORMAT_PCM8;

        union config config = {0};
        bool ok = ret->gpio->driver->config.get(ret->gpio->context, &config);
        ok = ok && config.gpio.pin(ret->gpio->context, ret->pin,
                                   DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);

        if (ok)
            *ctx = ret;
        else
            mem_del(ret);
    }
}

static void
clean(void *ctx)
{
    struct buzzer *bz = ctx;

    union config config = {0};
    bz->gpio->driver->config.get(bz->gpio->context, &config);
    config.gpio.pin(bz->gpio->context, bz->pin, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);

    mem_del(bz);
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct buzzer *bz = ctx;
    mem_copy(cfg, &(bz->config), sizeof(union config));
    return true;
}

static bool
stream_write(void *ctx, u32 idx, void *data)
{
    bool ret = true;

    if (idx == 0)
    {
        struct buzzer *bz = ctx;
        ret = pin_set(bz->gpio, bz->pin, *((u8*)data) >= UINT8_MAX / 2);
        usleep(bz->timer, 1000000000 / 48000);
    }
    else
        ret = false;

    return ret;
}

DECLARE_DRIVER(audio, buzzer)
{
    .init = init, .clean = clean,
    .config.get = config_get,
    .stream.write = stream_write
};
