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

#ifdef CONFIG_EXTRA_BITBANG

#include <stdlib.h>
#include <bitbang.h>

#include <h3/ports.h>

#include <interface/timer.h>

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

extern struct spi *
spi_new(enum pin ss, enum pin sck, enum pin mosi, enum pin miso)
{
    struct spi *ret = calloc(1, sizeof(struct spi));

    ret->ss = ss;
    ret->sck = sck;
    ret->mosi = mosi;
    ret->miso = miso;

    ret->delay = 0;
    ret->sleep = empty;

    pin_config(ret->ss, PIN_CFG_OUT);
    pin_config(ret->sck, PIN_CFG_OUT);
    pin_config(ret->mosi, PIN_CFG_OUT);
    pin_config(ret->miso, PIN_CFG_IN);

    return ret;
}

extern struct spi *
spi_del(struct spi *s)
{
    if (s != NULL)
    {
        pin_config(s->ss, PIN_CFG_OFF);
        pin_config(s->sck, PIN_CFG_OFF);
        pin_config(s->mosi, PIN_CFG_OFF);
        pin_config(s->miso, PIN_CFG_OFF);
        free(s);
    }

    return NULL;
}

extern void
spi_config(struct spi *s, u32 freq, u8 mode, bool lsb)
{
    s->cpha = (mode & 0x1);
    s->cpol = (mode & 0x2);
    s->lsb = lsb;

    if (freq == 0)
        s->sleep = empty;
    else if (freq <= TIMER_CLOCK / 2)
    {
        s->sleep = timer_csleep;
        s->delay = TIMER_CLOCK / freq / 2;
    }
    else
    {
        if (freq > 240000000)
            freq = 240000000;

        s->sleep = hsleep;
        s->delay = 480000000 / freq / 2;
    }
}

extern u8
spi_transfer(struct spi *s, u8 x)
{
    u8 ret = 0;

    pin_write(s->ss, false);
    pin_write(s->sck, (s->cpha) ? true : false);
    for (u8 i = 0; i < 8; i++)
    {
        if (s->lsb) ret >>= 1;
        else        ret <<= 1;
        ret |= pin_read(s->miso) << ((s->lsb) ? 7 : 0);

        bool bit = (s->lsb) ? x & 0x01 : x & 0x80;
        pin_write(s->mosi, (s->cpol) ? !bit : bit);
        s->sleep(s->delay);
        pin_write(s->sck, (s->cpha) ? false : true);
        s->sleep(s->delay);
        pin_write(s->sck, (s->cpha) ? true : false);

        if (s->lsb) x >>= 1;
        else        x <<= 1;
    }
    pin_write(s->ss, true);

    return (s->cpol) ? ~ret : ret;
}

#endif
