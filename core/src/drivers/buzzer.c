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
#include <stdlib.h>
#include <string.h>
#include <vermillion/drivers.h>

struct buzzer
{
    struct device *timer;
    struct device *gpio;
    u16 pin;

    union config config;
};

static void
init(void **ctx, struct device *gpio, u16 pin, struct device *timer)
{
    struct buzzer *ret = NULL;

    if (timer && gpio)
        ret = calloc(1, sizeof(struct buzzer));

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
            free(ret);
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

    free(bz);
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct buzzer *bz = ctx;
    memcpy(cfg, &(bz->config), sizeof(union config));
    return true;
}

static bool
stream_write(void *ctx, u8 data)
{
    bool ret = false;

    struct buzzer *bz = ctx;
    ret = pin_set(bz->gpio, bz->pin, data >= UINT8_MAX / 2);
    usleep(bz->timer, 1000000000 / 48000);

    return ret;
}

DECLARE_DRIVER(buzzer)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_STREAM,
    .type = DRIVER_TYPE_AUDIO,
    .config.get = config_get,
    .interface.stream.write = stream_write
};
