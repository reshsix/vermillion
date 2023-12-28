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

struct spi
{
    struct device *gpio;
    struct device *timer;

    u16 ss;
    u16 sck;
    u16 mosi;
    u16 miso;

    bool lsb;
    bool cpol;
    bool cpha;

    u32 delay;
    void (*sleep)(struct device *, u32);
};

static void
empty(struct device *tmr, u32 x)
{
    (void)(tmr), (void)(x);
    return;
}

static void
hsleep(struct device *tmr, u32 n)
{
    (void)(tmr);
    for (register u32 i = 0; i < (n / 4); i++)
        asm volatile ("nop");
}

static u8
spi_transfer(struct spi *spi, u8 x)
{
    u8 ret = 0;

    pin_set(spi->gpio, spi->ss, false);
    pin_set(spi->gpio, spi->sck, (spi->cpha) ? true : false);
    for (u8 i = 0; i < 8; i++)
    {
        if (spi->lsb) ret >>= 1;
        else          ret <<= 1;

        bool state = false;
        pin_get(spi->gpio, spi->miso, &state);
        ret |= state << ((spi->lsb) ? 7 : 0);

        bool bit = (spi->lsb) ? x & 0x01 : x & 0x80;
        pin_set(spi->gpio, spi->mosi, (spi->cpol) ? !bit : bit);
        spi->sleep(spi->timer->context, spi->delay);

        pin_set(spi->gpio, spi->sck, (spi->cpha) ? false : true);
        spi->sleep(spi->timer->context, spi->delay);
        pin_set(spi->gpio, spi->sck, (spi->cpha) ? true : false);

        if (spi->lsb) x >>= 1;
        else          x <<= 1;
    }
    pin_set(spi->gpio, spi->ss, true);

    return (spi->cpol) ? ~ret : ret;
}

static void
init(void **ctx, struct device *gpio, u16 ss, u16 sck, u16 mosi, u16 miso,
     struct device *timer)
{
    struct spi *ret = NULL;

    if (gpio && timer)
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
        ret->timer = timer;

        union config config = {0};
        ret->gpio->driver->config.get(ret->gpio->context, &config);
        config.gpio.pin(ret->gpio->context, ret->ss, DRIVER_GPIO_OUT,
                        DRIVER_GPIO_PULLOFF);
        config.gpio.pin(ret->gpio->context, ret->sck, DRIVER_GPIO_OUT,
                        DRIVER_GPIO_PULLOFF);
        config.gpio.pin(ret->gpio->context, ret->mosi, DRIVER_GPIO_OUT,
                        DRIVER_GPIO_PULLOFF);
        config.gpio.pin(ret->gpio->context, ret->miso, DRIVER_GPIO_IN,
                        DRIVER_GPIO_PULLOFF);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct spi *spi = ctx;

    union config config = {0};
    spi->gpio->driver->config.get(spi->gpio->context, &config);
    config.gpio.pin(spi->gpio->context, spi->ss, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);
    config.gpio.pin(spi->gpio->context, spi->sck, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);
    config.gpio.pin(spi->gpio->context, spi->mosi, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);
    config.gpio.pin(spi->gpio->context, spi->miso, DRIVER_GPIO_OFF,
                    DRIVER_GPIO_PULLOFF);

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
        cfg->spi.freq = clock(spi->timer) / (spi->delay * 2);

    return true;
}

static bool
config_set(void *ctx, union config *cfg)
{
    bool ret = false;

    if (cfg->spi.freq > 240000000)
    {
        struct spi *spi = ctx;
        spi->cpha = (cfg->spi.mode & 0x1);
        spi->cpol = (cfg->spi.mode & 0x2);
        spi->lsb = cfg->spi.lsb;

        u32 clk = clock(spi->timer);
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
stream_read(void *ctx, u32 idx, u8 *data)
{
    bool ret = (idx == 0);

    if (ret)
        data[0] = spi_transfer(ctx, 0x0);

    return ret;
}

static bool
stream_write(void *ctx, u32 idx, u8 *data)
{
    bool ret = (idx == 0);

    if (ret)
        spi_transfer(ctx, data[0]);

    return ret;
}

DECLARE_DRIVER(spi)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_STREAM,
    .type = DRIVER_TYPE_SPI,
    .config.get = config_get,
    .config.set = config_set,
    .stream.read = stream_read,
    .stream.write = stream_write
};
