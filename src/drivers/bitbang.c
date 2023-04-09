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

#include <stdlib.h>
#include <vermillion/drivers.h>

struct spi
{
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

static struct spi spi;

static void
empty(u32 x)
{
    (void)(x);
    return;
}

static void
hsleep(const u32 n)
{
    for (register u32 i = 0; i < (n / 4); i++)
        asm volatile ("nop");
}

static bool
init(void)
{
    spi.ss = CONFIG_BITBANG_SPI_SS;
    spi.sck = CONFIG_BITBANG_SPI_SCK;
    spi.mosi = CONFIG_BITBANG_SPI_MOSI;
    spi.miso = CONFIG_BITBANG_SPI_MISO;

    spi.delay = 0;
    spi.sleep = empty;

    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.cfgpin(spi.ss, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.cfgpin(spi.sck, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.cfgpin(spi.mosi, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.cfgpin(spi.miso, DRIVER_GPIO_IN, DRIVER_GPIO_PULLDOWN);

    return true;
}

static void
clean(void)
{
    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.cfgpin(spi.ss, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.cfgpin(spi.sck, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.cfgpin(spi.mosi, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.cfgpin(spi.miso, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
}

static bool
spi_config(u32 freq, u8 mode, bool lsb)
{
    spi.cpha = (mode & 0x1);
    spi.cpol = (mode & 0x2);
    spi.lsb = lsb;

    const struct driver *t0 = driver_find(DRIVER_TYPE_TIMER, 0);
    u32 clk = t0->routines.timer.clock();
    if (freq == 0)
        spi.sleep = empty;
    else if (freq <= clk / 2)
    {
        spi.sleep = t0->routines.timer.csleep;
        spi.delay = clk / freq / 2;
    }
    else
    {
        if (freq > 240000000)
            freq = 240000000;

        spi.sleep = hsleep;
        spi.delay = 480000000 / freq / 2;
    }

    return true;
}

static u8
spi_transfer(u8 x)
{
    u8 ret = 0;

    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.set(spi.ss, false);
    gpio->routines.gpio.set(spi.sck, (spi.cpha) ? true : false);
    for (u8 i = 0; i < 8; i++)
    {
        if (spi.lsb) ret >>= 1;
        else        ret <<= 1;
        ret |= gpio->routines.gpio.get(spi.miso) << ((spi.lsb) ? 7 : 0);

        bool bit = (spi.lsb) ? x & 0x01 : x & 0x80;
        gpio->routines.gpio.set(spi.mosi, (spi.cpol) ? !bit : bit);
        spi.sleep(spi.delay);
        gpio->routines.gpio.set(spi.sck, (spi.cpha) ? false : true);
        spi.sleep(spi.delay);
        gpio->routines.gpio.set(spi.sck, (spi.cpha) ? true : false);

        if (spi.lsb) x >>= 1;
        else        x <<= 1;
    }
    gpio->routines.gpio.set(spi.ss, true);

    return (spi.cpol) ? ~ret : ret;
}

static const struct driver bitbang_spi =
{
    .name = "bitbang-spi",
    .init = init, .clean = clean,
    .api = DRIVER_API_GENERIC,
    .type = DRIVER_TYPE_SPI,
    .routines.spi.config   = spi_config,
    .routines.spi.transfer = spi_transfer
};
driver_register(bitbang_spi);
