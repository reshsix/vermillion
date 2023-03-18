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

#ifdef CONFIG_VIDEO_ILI9488_SPI_X

#include <types.h>
#include <utils.h>
#include <stdarg.h>
#include <h3/ports.h>

struct ili9488
{
    enum pin dcrs;
    enum pin leds;

    void (*write)(u8);
};

/* Led pin is negated, so with a transistor in NOT configuration,
   and a capacitor between emitter and base, the screen doesn't
   turn white on boot */

static struct ili9488 *
ili9488_new(enum pin dcrs, enum pin leds, void (*write)(u8))
{
    struct ili9488 *ret = malloc(sizeof(struct ili9488));

    if (ret)
    {
        pin_config(dcrs, PIN_CFG_OUT);
        pin_config(leds, PIN_CFG_OUT);
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
        pin_config(ili->dcrs, PIN_CFG_OFF);
        pin_config(ili->leds, PIN_CFG_OFF);
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

    pin_write(ili->dcrs, false);
    ili->write(buf[0]);

    pin_write(ili->dcrs, true);
    for (u8 i = 1; i < c; i++)
        ili->write(buf[i]);
    pin_write(ili->dcrs, false);
}

static void
ili9488_command2(struct ili9488 *ili, u8 n, u8 *buf, size_t length)
{
    pin_write(ili->dcrs, false);
    ili->write(n);

    pin_write(ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        ili->write(buf[i]);
    pin_write(ili->dcrs, false);
}

static void
ili9488_command3(struct ili9488 *ili, u8 n, u8 c, size_t length)
{
    pin_write(ili->dcrs, false);
    ili->write(n);

    pin_write(ili->dcrs, true);
    for (size_t i = 0; i < length; i++)
        ili->write(c);
    pin_write(ili->dcrs, false);
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
    pin_config(ili->dcrs, PIN_CFG_OUT);
    pin_write(ili->leds, true);
    pin_config(ili->leds, PIN_CFG_OUT);
    pin_write(ili->leds, true);

    ili9488_command(ili, 1, 0x00, 0);
    ili9488_command(ili, 1, 0x01, 0);
    msleep(10);

    ili9488_command(ili, 1, 0x11);
    msleep(5);

    ili9488_command(ili, 2, 0x36, 0xE8);
    ili9488_command(ili, 2, 0x3A, 0x06);
    ili9488_clear(ili);

    if (splash != NULL)
        ili9488_update(ili, splash, (480 - splash_w) / 2, (320 - splash_h) / 2,
                       splash_w, splash_h);

    ili9488_command(ili, 1, 0x13);
    ili9488_command(ili, 1, 0x29);
    msleep(10);

    pin_write(ili->leds, false);
}

#endif

#ifdef CONFIG_VIDEO_ILI9488_SPI_X

#include <bitbang.h>

struct video
{
    struct spi *spi;
    struct ili9488 *ili;
};

struct video video;

extern void
_video_clean(void)
{
    spi_del(video.spi);
    ili9488_del(video.ili);
}

static struct spi *spi0 = NULL;
void spi0_write(u8 data){ spi_transfer(spi0, data); }
extern u8 _binary_splash_rgb_start[];
extern bool
_video_init(void)
{
    bool ret = false;

    video.spi = spi_new(CONFIG_ILI9488_SS, CONFIG_ILI9488_SCK,
                       CONFIG_ILI9488_MOSI, CONFIG_ILI9488_MISO);
    video.ili = ili9488_new(CONFIG_ILI9488_DCRS, CONFIG_ILI9488_LEDS,
                            spi0_write);
    if (video.spi && video.ili)
    {
        spi0 = video.spi;
        spi_config(spi0, SPI_MAX, 0, false);
        ili9488_start(video.ili, _binary_splash_rgb_start, 96, 96);
        ret = true;
    }
    else
        _video_clean();

    return ret;
}

extern void
video_update(u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    ili9488_update(video.ili, buffer, x, y, w, h);
}

extern void
video_clear(void)
{
    ili9488_clear(video.ili);
}

#endif
