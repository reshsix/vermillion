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

#ifdef CONFIG_SPI_BITBANG

#include <stdlib.h>

#include <h3/ports.h>

#include <interface/timer.h>

struct spi
{
    enum pin ss;
    enum pin sck;
    enum pin mosi;
    enum pin miso;

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

extern bool
_spi_init(void)
{
    spi.ss = CONFIG_BITBANG_SPI_SS;
    spi.sck = CONFIG_BITBANG_SPI_SCK;
    spi.mosi = CONFIG_BITBANG_SPI_MOSI;
    spi.miso = CONFIG_BITBANG_SPI_MISO;

    spi.delay = 0;
    spi.sleep = empty;

    pin_config(spi.ss, PIN_CFG_OUT);
    pin_config(spi.sck, PIN_CFG_OUT);
    pin_config(spi.mosi, PIN_CFG_OUT);
    pin_config(spi.miso, PIN_CFG_IN);

    return true;
}

extern void
_spi_clean(void)
{
    pin_config(spi.ss, PIN_CFG_OFF);
    pin_config(spi.sck, PIN_CFG_OFF);
    pin_config(spi.mosi, PIN_CFG_OFF);
    pin_config(spi.miso, PIN_CFG_OFF);
}

extern bool
spi_config(u32 freq, u8 mode, bool lsb)
{
    spi.cpha = (mode & 0x1);
    spi.cpol = (mode & 0x2);
    spi.lsb = lsb;

    if (freq == 0)
        spi.sleep = empty;
    else if (freq <= TIMER_CLOCK / 2)
    {
        spi.sleep = timer_csleep;
        spi.delay = TIMER_CLOCK / freq / 2;
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

extern u8
spi_transfer(u8 x)
{
    u8 ret = 0;

    pin_write(spi.ss, false);
    pin_write(spi.sck, (spi.cpha) ? true : false);
    for (u8 i = 0; i < 8; i++)
    {
        if (spi.lsb) ret >>= 1;
        else        ret <<= 1;
        ret |= pin_read(spi.miso) << ((spi.lsb) ? 7 : 0);

        bool bit = (spi.lsb) ? x & 0x01 : x & 0x80;
        pin_write(spi.mosi, (spi.cpol) ? !bit : bit);
        spi.sleep(spi.delay);
        pin_write(spi.sck, (spi.cpha) ? false : true);
        spi.sleep(spi.delay);
        pin_write(spi.sck, (spi.cpha) ? true : false);

        if (spi.lsb) x >>= 1;
        else        x <<= 1;
    }
    pin_write(spi.ss, true);

    return (spi.cpol) ? ~ret : ret;
}

#endif
