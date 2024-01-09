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

    u32 delay;
    void (*sleep)(u32);
};

static void
empty(u32 x)
{
    (void)(x);
    return;
}

static void
hsleep(u32 n)
{
    for (register u32 i = 0; i < (n / 4); i++)
        asm volatile ("nop");
}

static void
csleep(u32 n)
{
    if (n > 256)
    {
        for (u32 i = 0; i < n / 255; i++)
            wheel_sleep(WHEEL_INNER, 255);
        wheel_sleep(WHEEL_INNER, n % 255);
    }
    else
        wheel_sleep(WHEEL_INNER, n);
}

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
        spi->sleep(spi->delay);

        gpio_set(spi->gpio, spi->sck, (spi->cpha) ? false : true);
        spi->sleep(spi->delay);
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
        ret->ss = ss;
        ret->sck = sck;
        ret->mosi = mosi;
        ret->miso = miso;

        ret->delay = 0;
        ret->sleep = empty;

        ret->gpio = gpio;

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
    struct spi *spi = ctx;

    gpio_config(spi->gpio, spi->ss, GPIO_OFF, GPIO_PULLOFF);
    gpio_config(spi->gpio, spi->sck, GPIO_OFF, GPIO_PULLOFF);
    gpio_config(spi->gpio, spi->mosi, GPIO_OFF, GPIO_PULLOFF);
    gpio_config(spi->gpio, spi->miso, GPIO_OFF, GPIO_PULLOFF);

    mem_del(ctx);
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct spi *spi = ctx;
    cfg->spi.mode = spi->cpol << 1 | spi->cpha;
    cfg->spi.lsb = spi->lsb;

    if (spi->sleep == empty)
        cfg->spi.freq = 0;
    else if (spi->sleep == hsleep)
        cfg->spi.freq = 480000000 / (spi->delay * 2);
    else
        cfg->spi.freq = (1000000 / WHEEL_INNER_US) / (spi->delay * 2);

    return true;
}

static bool
config_set(void *ctx, union config *cfg)
{
    bool ret = false;

    if (cfg->spi.freq < 240000000)
    {
        struct spi *spi = ctx;
        spi->cpha = (cfg->spi.mode & 0x1);
        spi->cpol = (cfg->spi.mode & 0x2);
        spi->lsb = cfg->spi.lsb;

        u32 clk = 1000000 / WHEEL_INNER_US;
        if (cfg->spi.freq == 0)
            spi->sleep = empty;
        else if (cfg->spi.freq <= clk / 2)
        {
            spi->sleep = csleep;
            spi->delay = clk / cfg->spi.freq / 2;
        }
        else
        {
            spi->sleep = hsleep;
            spi->delay = 480000000 / cfg->spi.freq / 2;
        }

        ret = true;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *data)
{
    bool ret = (idx == 0);

    if (ret)
        *((u8*)data) = spi_transfer(ctx, 0x0);

    return ret;
}

static bool
write(void *ctx, u32 idx, void *data)
{
    bool ret = (idx == 0);

    if (ret)
        spi_transfer(ctx, *((u8*)data));

    return ret;
}

drv_decl (spi, spi)
{
    .init = init, .clean = clean,
    .config.get = config_get,
    .config.set = config_set,
    .read = read, .write = write
};
