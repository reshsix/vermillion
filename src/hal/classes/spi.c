/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <general/types.h>

#include <hal/bus.h>
#include <hal/classes/spi.h>

extern bool
spi_info(dev_spi *ds, u32 *freq, enum spi_mode *mode, bool *lsb)
{
    struct spi_cfg cfg = {0};

    bool ret = bus_ioctl((dev_bus *)ds, SPI_CONFIG_GET, &cfg);
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
    return bus_ioctl((dev_bus *)ds, SPI_CONFIG_SET, &cfg);
}

extern bool
spi_state(dev_spi *ds, bool cs)
{
    return bus_ioctl((dev_bus *)ds, SPI_STATE_CS, (void*)cs);
}

extern bool
spi_limit(dev_spi *ds, size_t *count)
{
    bool ret = false;

    size_t w = 0, c = 0;
    ret = bus_stat((dev_bus *)ds, &w, &c);
    if (ret)
    {
        if (count)
            *count = c;
    }

    return ret;
}

extern bool
spi_transfer(dev_spi *ds, void *data, size_t count)
{
    return bus_transfer((dev_bus *)ds, data, count);
}

extern bool
spi_poll(dev_spi *ds)
{
    return bus_poll((dev_bus *)ds);
}
