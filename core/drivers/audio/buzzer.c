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
#include <core/wheel.h>

#include <core/gpio.h>
#include <core/audio.h>

struct buzzer
{
    dev_gpio *gpio;
    u16 pin;

    union config config;
};

static void
init(void **ctx, dev_gpio *gpio, u16 pin)
{
    struct buzzer *ret = NULL;

    if (gpio)
        ret = mem_new(sizeof(struct buzzer));

    if (ret)
    {
        ret->gpio = gpio;
        ret->pin = pin;

        ret->config.audio.freq = 48000;
        ret->config.audio.format = DRIVER_AUDIO_FORMAT_PCM8;

        if (gpio_config(ret->gpio, ret->pin, GPIO_OUT, GPIO_PULLOFF))
            *ctx = ret;
        else
            mem_del(ret);
    }
}

static void
clean(void *ctx)
{
    struct buzzer *bz = ctx;
    gpio_config(bz->gpio, bz->pin, GPIO_OFF, GPIO_PULLOFF);
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
write(void *ctx, u32 idx, void *data)
{
    bool ret = true;

    if (idx == 0)
    {
        struct buzzer *bz = ctx;
        ret = gpio_set(bz->gpio, bz->pin, *((u8*)data) >= UINT8_MAX / 2);
        wheel_sleep(WHEEL_INNER, WHEEL_INNER_US / 48000);
    }
    else
        ret = false;

    return ret;
}

drv_decl (audio, buzzer)
{
    .init = init, .clean = clean,
    .config.get = config_get,
    .write = write
};
