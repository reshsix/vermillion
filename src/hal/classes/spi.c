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

#include <general/types.h>

#include <hal/generic/stream.h>
#include <hal/classes/spi.h>

extern bool
spi_read(dev_spi *ds, u8 *data)
{
    return stream_read((dev_stream *)ds, STREAM_COMMON, data);
}

extern bool
spi_write(dev_spi *ds, u8 data)
{
    return stream_write((dev_stream *)ds, STREAM_COMMON, &data);
}

extern bool
spi_info(dev_spi *ds, u32 *freq, enum spi_mode *mode, bool *lsb)
{
    struct spi_cfg cfg = {0};

    bool ret = stream_read((dev_stream *)ds, SPI_CONFIG, &cfg);
    if (ret)
    {
        if (freq) *freq = cfg.freq;
        if (mode) *mode = cfg.mode;
        if (lsb)  *lsb  = cfg.lsb;
    }

    return ret;
}

extern bool
spi_config(dev_spi *ds, u32 freq, enum spi_mode mode, bool lsb)
{
    struct spi_cfg cfg = {.freq = freq, .mode = mode, .lsb = lsb};
    return stream_write((dev_stream *)ds, SPI_CONFIG, &cfg);
}
