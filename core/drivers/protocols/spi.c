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

#include <core/spi.h>
#include <core/gpio.h>

struct spi
{
    dev_gpio *gpio;

    u16 ss;
    u16 sck;
    u16 mosi;
    u16 miso;

    bool lsb;
    bool cpol;
    bool cpha;

    u8 delay;
};

static u8
spi_transfer(struct spi *spi, u8 x)
{
    u8 ret = 0;

    gpio_set(spi->gpio, spi->ss, false);
    gpio_set(spi->gpio, spi->sck, (spi->cpha) ? true : false);
    for (u8 i = 0; i < 8; i++)
    {
        if (spi->lsb) ret >>= 1;
        else          ret <<= 1;

        bool state = false;
        gpio_get(spi->gpio, spi->miso, &state);
        ret |= state << ((spi->lsb) ? 7 : 0);

        bool bit = (spi->lsb) ? x & 0x01 : x & 0x80;
        gpio_set(spi->gpio, spi->mosi, (spi->cpol) ? !bit : bit);
        wheel_sleep(WHEEL_INNER, spi->delay);

        gpio_set(spi->gpio, spi->sck, (spi->cpha) ? false : true);
        wheel_sleep(WHEEL_INNER, spi->delay);
        gpio_set(spi->gpio, spi->sck, (spi->cpha) ? true : false);

        if (spi->lsb) x >>= 1;
        else          x <<= 1;
    }
    gpio_set(spi->gpio, spi->ss, true);

    return (spi->cpol) ? ~ret : ret;
}

static void
init(void **ctx, dev_gpio *gpio, u16 ss, u16 sck, u16 mosi, u16 miso)
{
    struct spi *ret = NULL;

    if (gpio)
        ret = mem_new(sizeof(struct spi));

    if (ret)
    {
        ret->gpio = gpio;

        ret->ss = ss;
        ret->sck = sck;
        ret->mosi = mosi;
        ret->miso = miso;

        ret->delay = 1;

        gpio_config(ret->gpio, ret->ss, GPIO_OUT, GPIO_PULLOFF);
        gpio_config(ret->gpio, ret->sck, GPIO_OUT, GPIO_PULLOFF);
        gpio_config(ret->gpio, ret->mosi, GPIO_OUT, GPIO_PULLOFF);
        gpio_config(ret->gpio, ret->miso, GPIO_IN, GPIO_PULLOFF);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    if (ctx)
    {
        struct spi *spi = ctx;
        gpio_config(spi->gpio, spi->ss, GPIO_OFF, GPIO_PULLOFF);
        gpio_config(spi->gpio, spi->sck, GPIO_OFF, GPIO_PULLOFF);
        gpio_config(spi->gpio, spi->mosi, GPIO_OFF, GPIO_PULLOFF);
        gpio_config(spi->gpio, spi->miso, GPIO_OFF, GPIO_PULLOFF);
    }

    mem_del(ctx);
}

static bool
stat(void *ctx, u32 idx, u32 *width)
{
    bool ret = true;

    if (ctx)
    {
        switch (idx)
        {
            case 0:
                *width = sizeof(struct spi_cfg);
                break;
            case 1:
                *width = sizeof(u8);
                break;
            default:
                ret = false;
                break;
        }
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    if (ctx)
    {
        struct spi *spi = ctx;
        switch (idx)
        {
            case 0:
                ret = true;

                struct spi_cfg cfg = {0};
                cfg.mode = spi->cpol << 1 | spi->cpha;
                cfg.lsb = spi->lsb;
                cfg.freq = 1000000 / (WHEEL_INNER_US * spi->delay);
                mem_copy(data, &cfg, sizeof(struct spi_cfg));
                break;

            case 1:
                ret = true;

                u8 byte = spi_transfer(ctx, 0x0);
                mem_copy(data, &byte, sizeof(u8));
                break;
        }
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    if (ctx)
    {
        struct spi *spi = ctx;
        switch (idx)
        {
            case 0:;
                struct spi_cfg cfg = {0};
                mem_copy(&cfg, data, sizeof(struct spi_cfg));
                ret = (cfg.freq <= 100000 && cfg.freq >= 391 && cfg.mode < 4);

                if (ret)
                {
                    struct spi *spi = ctx;
                    spi->cpha = (cfg.mode & 0x1);
                    spi->cpol = (cfg.mode & 0x2);
                    spi->lsb = cfg.lsb;
                    spi->delay = (1000000 / (cfg.freq * WHEEL_INNER_US));
                }
                break;

            case 1:
                ret = true;

                u8 byte = 0;
                mem_copy(&byte, data, sizeof(u8));
                spi_transfer(spi, byte);
                break;
        }
    }

    return ret;
}

drv_decl (spi, spi)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
