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
#include <stdarg.h>
#include <stdlib.h>
#include <vermillion/drivers.h>

struct ili9488
{
    u16 dcrs;
    u16 leds;

    void (*write)(u8);
};

/* Led pin is negated, so with a transistor in NOT configuration,
   and a capacitor between emitter and base, the screen doesn't
   turn white on boot */

static struct ili9488 *
ili9488_new(u16 dcrs, u16 leds, void (*write)(u8))
{
    struct ili9488 *ret = malloc(sizeof(struct ili9488));

    if (ret)
    {
        const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
        gpio->routines.gpio.cfgpin(dcrs, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
        gpio->routines.gpio.cfgpin(leds, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
        ret->dcrs = dcrs;
        ret->leds = leds;

        ret->write = write;
    }

    return ret;
}

static struct ili9488 *
ili9488_del(struct ili9488 *ili)
{
    if (!ili)
    {
        const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
        gpio->routines.gpio.cfgpin(ili->dcrs, DRIVER_GPIO_OFF,
                                   DRIVER_GPIO_PULLOFF);
        gpio->routines.gpio.cfgpin(ili->leds, DRIVER_GPIO_OFF,
                                   DRIVER_GPIO_PULLOFF);
        free(ili);
    }

    return NULL;
}

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

    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.set(ili->dcrs, false);
    ili->write(buf[0]);

    gpio->routines.gpio.set(ili->dcrs, true);
    for (u8 i = 1; i < c; i++)
        ili->write(buf[i]);
    gpio->routines.gpio.set(ili->dcrs, false);
}

static void
ili9488_command2(struct ili9488 *ili, u8 n, u8 *buf, size_t length)
{
    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.set(ili->dcrs, false);
    ili->write(n);

    gpio->routines.gpio.set(ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        ili->write(buf[i]);
    gpio->routines.gpio.set(ili->dcrs, false);
}

static void
ili9488_command3(struct ili9488 *ili, u8 n, u8 c, size_t length)
{
    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.set(ili->dcrs, false);
    ili->write(n);

    gpio->routines.gpio.set(ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        ili->write(c);
    gpio->routines.gpio.set(ili->dcrs, false);
}

static void
ili9488_update(struct ili9488 *ili, u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    u16 ex = w + x - 1;
    u16 ey = h + y - 1;

    if (x < ex && y < ey)
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
    ili9488_command3(ili, 0x2c, 0x0, 480 * 320 * 3);
    ili9488_command(ili, 1, 0x00);
}

static void
ili9488_start(struct ili9488 *ili, u8 *splash, u16 splash_w, u16 splash_h)
{
    const struct driver *t0 = driver_find(DRIVER_TYPE_TIMER, 0);
    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);

    gpio->routines.gpio.cfgpin(ili->dcrs, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.set(ili->leds, true);
    gpio->routines.gpio.cfgpin(ili->leds, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.set(ili->leds, true);

    ili9488_command(ili, 1, 0x00, 0);
    ili9488_command(ili, 1, 0x01, 0);
    t0->routines.timer.msleep(10);

    ili9488_command(ili, 1, 0x11);
    t0->routines.timer.msleep(5);

    ili9488_command(ili, 2, 0x36, 0xE8);
    ili9488_command(ili, 2, 0x3A, 0x06);
    ili9488_clear(ili);

    if (splash != NULL)
        ili9488_update(ili, splash, (480 - splash_w) / 2, (320 - splash_h) / 2,
                       splash_w, splash_h);

    ili9488_command(ili, 1, 0x13);
    ili9488_command(ili, 1, 0x29);
    t0->routines.timer.msleep(10);

    gpio->routines.gpio.set(ili->leds, false);
}

struct video
{
    struct ili9488 *ili;
};

static struct video video;

static void
clean(void)
{
    ili9488_del(video.ili);
}

static struct driver *spi0 = NULL;
static void spi_write(u8 data){ spi0->routines.spi.transfer(data); }
static bool
init(void)
{
    bool ret = false;

    video.ili = ili9488_new(CONFIG_ILI9488_DCRS, CONFIG_ILI9488_LEDS,
                            spi_write);
    if (video.ili)
    {
        spi0 = driver_find(DRIVER_TYPE_SPI, 0);
        spi0->routines.spi.config(DRIVER_SPI_MAX, 0, false);
        ili9488_start(video.ili, NULL, 0, 0);
        ret = true;
    }
    else
        clean();

    return ret;
}

static void
video_info(u16 *width, u16 *height)
{
    if (width)
        *width = 480;
    if (height)
        *height = 320;
}

static void
video_update(u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    ili9488_update(video.ili, buffer, x, y, w, h);
}

static void
video_clear(void)
{
    ili9488_clear(video.ili);
}

static const struct driver ili9488_spi =
{
    .name = "ili9488-spi",
    .init = init, .clean = clean,
    .api = DRIVER_API_GENERIC,
    .type = DRIVER_TYPE_VIDEO,
    .routines.video.info   = video_info,
    .routines.video.update = video_update,
    .routines.video.clear  = video_clear
};
driver_register(ili9488_spi);
