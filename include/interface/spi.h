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

#ifndef INTERFACE_SPI_H
#define INTERFACE_SPI_H

#include <_types.h>
#include <drivers/dummy.h>
#include <drivers/bitbang.h>

bool _spi_init(void);
void _spi_clean(void);

bool spi_config(u32 freq, u8 mode, bool lsb);
u8 spi_transfer(u8 x);

#endif
