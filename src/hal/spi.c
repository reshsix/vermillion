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

#define VERMILLION_INTERNALS
#include <vermillion/hal/spi.h>
#include <vermillion/util/types.h>

/* Devtree setup */

static dev_spi *dev_l = NULL;
static uint8_t dev_c = 0;

extern void
spi_setup(dev_spi *list, uint8_t count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define SPI_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
vrm_spi_info(uint8_t id, uint32_t *freq, uint32_t *fields)
{
    uint32_t freq2 = 0, fields2 = 0;

    bool ret = SPI_CALL(info, &freq2, &fields2);
    if (ret)
    {
        if (freq)
            *freq = freq2;
        if (fields)
            *fields = fields2;
    }

    return ret;
}

extern bool
vrm_spi_config(uint8_t id, uint32_t freq, uint32_t fields)
{
    return SPI_CALL(config, freq, fields);
}

extern bool
vrm_spi_limit(uint8_t id, size_t *count)
{
    return SPI_CALL(limit, count);
}

extern bool
vrm_spi_transfer(uint8_t id, uint8_t *data, size_t count, uint32_t flags)
{
    bool ret = false;

    if (flags & VRM_SPI_NOWAIT)
        ret = SPI_CALL(transfer, data, count, flags);
    else
    {
        size_t limit = 0;
        ret = vrm_spi_limit(id, &limit);
        if (ret)
        {
            uint32_t flags2 = flags | VRM_SPI_PARTIAL;
            for (size_t i = 0; i < count; i += limit)
            {
                if (i + limit >= count)
                    flags2 = flags;

                size_t remain = count - i;
                size_t size = (remain > limit) ? limit : remain;
                while (!SPI_CALL(transfer, &(data[i]), size, flags2));
                while (!vrm_spi_poll(id));
            }
        }
    }

    return ret;
}

extern bool
vrm_spi_poll(uint8_t id)
{
    return SPI_CALL(poll);
}
