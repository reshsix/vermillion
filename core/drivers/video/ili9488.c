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
#include <core/video.h>
#include <core/stream.h>

struct ili9488
{
    dev_gpio *gpio;
    u16 dcrs;
    u16 leds;

    dev_stream *stream;

    u8 buffer24[480 * 3];
    u8 *buffer32[480 * 320 * 4];

    struct video_fb fb;
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

    gpio_set(ili->gpio, ili->dcrs, false);
    stream_write(ili->stream, 0, &(buf[0]));

    gpio_set(ili->gpio, ili->dcrs, true);
    for (u8 i = 1; i < c; i++)
        stream_write(ili->stream, 0, &(buf[i]));
    gpio_set(ili->gpio, ili->dcrs, false);
}

static void
ili9488_command2(struct ili9488 *ili, u8 n, u8 *buf, size_t length)
{
    gpio_set(ili->gpio, ili->dcrs, false);
    stream_write(ili->stream, 0, &n);

    gpio_set(ili->gpio, ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        stream_write(ili->stream, 0, &(buf[i]));
    gpio_set(ili->gpio, ili->dcrs, false);
}

static void
ili9488_command3(struct ili9488 *ili, u8 n, u8 c, size_t length)
{
    gpio_set(ili->gpio, ili->dcrs, false);
    stream_write(ili->stream, 0, &n);

    gpio_set(ili->gpio, ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        stream_write(ili->stream, 0, &c);
    gpio_set(ili->gpio, ili->dcrs, false);
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
    gpio_config(ili->gpio, ili->dcrs, GPIO_OUT, GPIO_PULLOFF);
    gpio_set(ili->gpio, ili->leds, true);
    gpio_config(ili->gpio, ili->leds, GPIO_OUT, GPIO_PULLOFF);
    gpio_set(ili->gpio, ili->leds, true);

    ili9488_command(ili, 1, 0x00, 0);
    ili9488_command(ili, 1, 0x01, 0);
    wheel_sleep(WHEEL_OUTER, 1);

    ili9488_command(ili, 1, 0x11);
    wheel_sleep(WHEEL_OUTER, 1);

    ili9488_command(ili, 2, 0x36, 0xE8);
    ili9488_command(ili, 2, 0x3A, 0x06);
    ili9488_clear(ili);

    ili9488_command(ili, 1, 0x13);
    ili9488_command(ili, 1, 0x29);
    wheel_sleep(WHEEL_OUTER, 1);

    gpio_set(ili->gpio, ili->leds, false);
}

static void
init(void **ctx, dev_gpio *gpio, u16 dcrs, u16 leds, dev_stream *stream)
{
    struct ili9488 *ret = NULL;

    if (gpio && stream)
        ret = mem_new(sizeof(struct ili9488));

    if (ret)
    {
        ret->gpio = gpio;

        gpio_config(ret->gpio, dcrs, GPIO_OUT, GPIO_PULLOFF);
        gpio_config(ret->gpio, leds, GPIO_OUT, GPIO_PULLOFF);

        ret->dcrs = dcrs;
        ret->leds = leds;

        ret->stream = stream;

        ili9488_start(ret);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct ili9488 *ili = ctx;

    gpio_config(ili->gpio, ili->dcrs, GPIO_OFF, GPIO_PULLOFF);
    gpio_config(ili->gpio, ili->leds, GPIO_OFF, GPIO_PULLOFF);

    mem_del(ili);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *depth)
{
    bool ret = true;

    if (ctx)
    {
        switch (idx)
        {
            case 0:
                *width = sizeof(struct video_fb);
                *depth = 1;
                break;
            case 1:
                *width = 480 * 4;
                *depth = 320;
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
    bool ret = false;

    if (ctx)
    {
        struct ili9488 *ili = ctx;
        switch (idx)
        {
            case 0:
                ret = (block == 0);

                if (ret)
                    mem_copy(buffer, &(ili->fb), sizeof(struct video_fb));
                break;

            case 1:
                ret = (block < 320);

                if (ret)
                    mem_copy(buffer, &(ili->buffer32[block * 480 * 4]),
                             480 * 4);
                break;
        }
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    if (ctx)
    {
        struct ili9488 *ili = ctx;
        switch (idx)
        {
            case 1:
                ret = (block < 320);

                if (ret)
                {
                    u8 *buffer8 = buffer;

                    bool diff = false;
                    for (u16 i = 0; !diff && i < 480; i++)
                        diff = mem_comp(&(ili->buffer32[i * 4]),
                                        &(buffer8[i * 4]), 3);

                    if (diff)
                    {
                        mem_copy(&(ili->buffer32[block * 480 * 4]),
                                 buffer, 480 * 4);

                        u32 index = block * 480;
                        for (u16 i = 0; i < 480; i++)
                            mem_copy(&(ili->buffer24[i * 3]),
                                     &(buffer8[i * 4]), 3);

                        u32 x = index % 480;
                        u32 y = index / 480;
                        u32 w = (x + 480 < 480) ? 480 : 480 - x;
                        u32 h = 1;

                        ili9488_update(ili, ili->buffer24, x, y, w, h);
                        if (w < 480)
                            ili9488_update(ili, &(ili->buffer24[w * 3]),
                                           0, y + 1, 480 - w, h);
                    }
                }
                break;
        }
    }

    return ret;
}

drv_decl (video, ili9488)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
