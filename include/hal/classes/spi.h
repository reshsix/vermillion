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

#pragma once

#include <hal/base/drv.h>
#include <hal/base/dev.h>
#include <hal/generic/stream.h>

enum spi_index
{
    SPI_CONFIG = STREAM_COMMON + 1,
};

drv_typedef (stream, spi);
dev_typedef (spi);

enum spi_mode
{
    SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3
};

struct [[gnu::packed]] spi_cfg
{
    u32 freq;
    enum spi_mode mode;
    bool lsb;
};

bool spi_read(dev_spi *ds, u8 *data);
bool spi_write(dev_spi *ds, u8 data);
bool spi_info(dev_spi *ds, u32 *freq, enum spi_mode *mode, bool *lsb);
bool spi_config(dev_spi *ds, u32 freq, enum spi_mode mode, bool lsb);
