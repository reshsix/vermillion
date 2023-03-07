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

#ifndef _BITBANG_H
#define _BITBANG_H

#include <types.h>
#include <h3/ports.h>

#define SPI_MAX 0

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

struct spi *spi_new(enum pin ss, enum pin sck, enum pin mosi, enum pin miso);
struct spi *spi_del(struct spi *s);
void spi_config(struct spi *s, u32 freq, u8 mode, bool lsb);
u8 spi_transfer(struct spi *s, u8 x);

#endif
