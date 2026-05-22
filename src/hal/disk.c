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
#include <vermillion/hal/disk.h>

/* Devtree setup */

static dev_disk *dev_l = NULL;
static u8 dev_c = 0;

extern void
disk_setup(dev_disk *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define DISK_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
disk_size(u8 id, u16 *sector, u32 *count)
{
    u16 sector2 = 0;
    u32 count2  = 0;

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
disk_read(u8 id, u8 *data, u32 block, u32 flags)
{
    (void)flags;
    return DISK_CALL(read, data, block);
}

extern bool
disk_write(u8 id, u8 *data, u32 block, u32 flags)
{
    (void)flags;
    return DISK_CALL(write, data, block);
}

/* ABI definitions */

static struct vrm_disk_v1 v1 =
{
    .size = disk_size, .read = disk_read, .write  = disk_write
};

extern void *
disk_driver(u8 version)
{
    void *ret = NULL;

    switch (version)
    {
        case VRM_DISK_V1:
            ret = &v1;
            break;
    }

    return ret;
}
