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

#define VERMILLION_INTERNALS
#include <vermillion/hal/spi.h>

/* Devtree setup */

static dev_spi *dev_l = NULL;
static u8 dev_c = 0;

extern void
spi_setup(dev_spi *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define SPI_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
spi_info(u8 id, u32 *freq, u8 *mode, bool *lsb)
{
    return SPI_CALL(info, freq, mode, lsb);
}

extern bool
spi_config(u8 id, u32 freq, u8 mode, bool lsb)
{
    return SPI_CALL(config, freq, mode, lsb);
}

extern bool
spi_begin(u8 id)
{
    return SPI_CALL(begin);
}

extern bool
spi_end(u8 id)
{
    return SPI_CALL(end);
}

extern bool
spi_limit(u8 id, size_t *count)
{
    return SPI_CALL(limit, count);
}

extern bool
spi_transfer(u8 id, u8 *data, size_t count)
{
    return SPI_CALL(transfer, data, count);
}

extern bool
spi_poll(u8 id)
{
    return SPI_CALL(poll);
}

/* ABI definitions */

static struct vrm_spi_v1 v1 =
{
    .info     = spi_info,     .config = spi_config,
    .begin    = spi_begin,    .end    = spi_end,
    .limit    = spi_limit,
    .transfer = spi_transfer, .poll   = spi_poll
};

extern void *
spi_driver(u8 version)
{
    void *ret = NULL;

    switch (version)
    {
        case VRM_SPI_V1:
            ret = &v1;
            break;
    }

    return ret;
}
