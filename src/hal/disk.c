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
#include <vermillion/hal/disk.h>
#include <vermillion/util/types.h>

/* Devtree setup */

static dev_disk *dev_l = NULL;
static uint8_t dev_c = 0;

extern void
disk_setup(dev_disk *list, uint8_t count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define DISK_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
vrm_disk_size(uint8_t id, uint16_t *sector, uint32_t *count)
{
    uint16_t sector2 = 0;
    uint32_t count2  = 0;

    bool ret = DISK_CALL(size, &sector2, &count2);
    if (ret)
    {
        if (sector)
            *sector = sector2;
        if (count)
            *count  = count2;
    }

    return ret;
}

extern bool
vrm_disk_read(uint8_t id, uint8_t *data, uint32_t block, uint32_t flags)
{
    (void)flags;
    return DISK_CALL(read, data, block);
}

extern bool
vrm_disk_write(uint8_t id, uint8_t *data, uint32_t block, uint32_t flags)
{
    (void)flags;
    return DISK_CALL(write, data, block);
}
