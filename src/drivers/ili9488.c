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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <vermillion/drivers.h>

struct ili9488
{
    struct device *gpio;
    u16 dcrs;
    u16 leds;

    struct device *stream;

    u8 buffer24[(0x200 / 4) * 3];
    u8 *buffer32[480 * 320 * 4];

    struct device *timer;
};

/* Led pin is negated, so with a transistor in NOT configuration,
   and a capacitor between emitter and base, the screen doesn't
   turn white on boot */

static void
ili9488_command(struct ili9488 *ili, u8 c, ...)
{
    if (c > 16)
        c = 16;

    va_list args;
    va_start(args, c);

    u8 buf[16];
    for (u8 i = 0; i < c; i++)
        buf[i] = va_arg(args, int);

    pin_set(ili->gpio, ili->dcrs, false);
    ili->stream->driver->interface.stream.write(ili->stream->context, buf[0]);

    pin_set(ili->gpio, ili->dcrs, true);
    for (u8 i = 1; i < c; i++)
        ili->stream->driver->interface.stream.write(ili->stream->context,
                                                    buf[i]);
    pin_set(ili->gpio, ili->dcrs, false);
}

static void
ili9488_command2(struct ili9488 *ili, u8 n, u8 *buf, size_t length)
{
    pin_set(ili->gpio, ili->dcrs, false);
    ili->stream->driver->interface.stream.write(ili->stream->context, n);

    pin_set(ili->gpio, ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        ili->stream->driver->interface.stream.write(ili->stream->context,
                                                    buf[i]);
    pin_set(ili->gpio, ili->dcrs, false);
}

static void
ili9488_command3(struct ili9488 *ili, u8 n, u8 c, size_t length)
{
    pin_set(ili->gpio, ili->dcrs, false);
    ili->stream->driver->interface.stream.write(ili->stream->context, n);

    pin_set(ili->gpio, ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        ili->stream->driver->interface.stream.write(ili->stream->context, c);
    pin_set(ili->gpio, ili->dcrs, false);
}

static void
ili9488_update(struct ili9488 *ili, u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    u16 ex = w + x - 1;
    u16 ey = h + y - 1;

    if (x <= ex && y <= ey)
    {
        u32 size = w * h * 3;
        ili9488_command(ili, 5, 0x2A, x >> 8, x & 0xFF, ex >> 8, ex & 0xFF);
        ili9488_command(ili, 5, 0x2B, y >> 8, y & 0xFF, ey >> 8, ey & 0xFF);
        ili9488_command2(ili, 0x2c, buffer, size);
        ili9488_command(ili, 1, 0x00);
    }
}

static void
ili9488_clear(struct ili9488 *ili)
{
    ili9488_command(ili, 5, 0x2A, 0, 0, 480 >> 8, 480 & 0xFF);
    ili9488_command(ili, 5, 0x2B, 0, 0, 320 >> 8, 320 & 0xFF);
    ili9488_command3(ili, 0x2c, 0x00, 480 * 320 * 3);
    ili9488_command(ili, 1, 0x00);
}

static void
ili9488_start(struct ili9488 *ili)
{
    union config config = {0};
    ili->gpio->driver->config.get(ili->gpio->context, &config);

    config.gpio.pin(ili->gpio->context, ili->dcrs, DRIVER_GPIO_OUT,
                    DRIVER_GPIO_PULLOFF);
    pin_set(ili->gpio, ili->leds, true);
    config.gpio.pin(ili->gpio->context, ili->leds, DRIVER_GPIO_OUT,
                    DRIVER_GPIO_PULLOFF);
    pin_set(ili->gpio, ili->leds, true);

    ili9488_command(ili, 1, 0x00, 0);
    ili9488_command(ili, 1, 0x01, 0);
    msleep(ili->timer, 10);

    ili9488_command(ili, 1, 0x11);
    msleep(ili->timer, 5);

    ili9488_command(ili, 2, 0x36, 0xE8);
    ili9488_command(ili, 2, 0x3A, 0x06);
    ili9488_clear(ili);

    ili9488_command(ili, 1, 0x13);
    ili9488_command(ili, 1, 0x29);
    msleep(ili->timer, 10);

    pin_set(ili->gpio, ili->leds, false);
}

static void
init(void **ctx, struct device *gpio, u16 dcrs, u16 leds,
     struct device *stream, struct device *timer)
{
    struct ili9488 *ret = NULL;

    if (gpio && stream)
        ret = calloc(1, sizeof(struct ili9488));

    if (ret)
    {
        ret->gpio = gpio;

        union config config = {0};
        ret->gpio->driver->config.get(ret->gpio->context, &config);
        config.gpio.pin(ret->gpio->context, dcrs, DRIVER_GPIO_OUT,
                        DRIVER_GPIO_PULLOFF);
        config.gpio.pin(ret->gpio->context, leds, DRIVER_GPIO_OUT,
                        DRIVER_GPIO_PULLOFF);

        ret->dcrs = dcrs;
        ret->leds = leds;

        ret->stream = stream;
        ret->timer = timer;

        ili9488_start(ret);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct ili9488 *ili = ctx;

    union config config = {0};
    ili->gpio->driver->config.get(ili->gpio->context, &config);
    config.gpio.pin(ili->gpio->context, ili->dcrs, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);
    config.gpio.pin(ili->gpio->context, ili->leds, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);

    free(ili);
}

static bool
config_get(void *ctx, union config *data)
{
    (void)(ctx);

    data->video.width = 480;
    data->video.height = 320;
    data->video.format = DRIVER_VIDEO_FORMAT_RGBX32;

    return true;
}

static bool
block_read(void *ctx, u8 *buffer, u32 block)
{
    bool ret = false;

    if (block < (480 * 320 * 4) / 0x200)
    {
        struct ili9488 *ili = ctx;
        memcpy(buffer, &(ili->buffer32[block * 0x200]), 0x200);
        ret = true;
    }

    return ret;
}

static bool
block_write(void *ctx, u8 *buffer, u32 block)
{
    bool ret = false;

    if (block < (480 * 320 * 4) / 0x200)
    {
        struct ili9488 *ili = ctx;
        const u8 size = 0x200 / 4;

        bool diff = false;
        for (u8 i = 0; !diff && i < size; i++)
            diff = memcmp(&(ili->buffer32[i * 4]), &(buffer[i * 4]), 3);

        if (diff)
        {
            memcpy(&(ili->buffer32[block * 0x200]), buffer, 0x200);

            u32 index = block * size;
            for (u8 i = 0; i < size; i++)
                memcpy(&(ili->buffer24[i * 3]), &(buffer[i * 4]), 3);

            u32 x = index % 480;
            u32 y = index / 480;
            u32 w = (x + size < 480) ? size : 480 - x;
            u32 h = 1;

            ili9488_update(ili, ili->buffer24, x, y, w, h);
            if (w < size)
                ili9488_update(ili, &(ili->buffer24[w * 3]), 0, y + 1,
                               size - w, h);
        }

        ret = true;
    }

    return ret;
}

static const struct driver ili9488_spi =
{
    .name = "ili9488",
    .init = init, .clean = clean,
    .api = DRIVER_API_BLOCK,
    .type = DRIVER_TYPE_VIDEO,
    .config.get = config_get,
    .interface.block.read = block_read,
    .interface.block.write = block_write
};
driver_register(ili9488_spi);
