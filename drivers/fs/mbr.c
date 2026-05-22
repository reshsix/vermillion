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
#include <general/mem.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/disk.h>

/* Driver definition */

struct mbr
{
    u8 disk;
    u32 lba;

    u16 sector;
    u32 count;
};

static struct mbr mbrs[1] = {0};

static bool
size(void *ctx, u16 *sector, u32 *count)
{
    struct mbr *mbr = ctx;
    *sector = mbr->sector;
    *count  = mbr->count;

    return true;
}

static bool
read(void *ctx, u8 *data, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if (block < mbr->count)
        ret = disk_read(mbr->disk, data, block + mbr->lba, 0);

    return ret;
}

static bool
write(void *ctx, u8 *data, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if (block < mbr->count)
        ret = disk_write(mbr->disk, data, block + mbr->lba, 0);

    return ret;
}

static const drv_disk mbr =
{
    .size = size, .read = read, .write = write
};

/* Device creation */

extern dev_disk
mbr_init(u8 id, u8 disk, u8 partition)
{
    struct mbr *ret = NULL;

    if (id < (sizeof(mbrs) / sizeof(struct mbr)) &&
        partition > 0 && partition < 5)
        ret = &(mbrs[id]);

    if (ret && !disk_size(disk, &(ret->sector), &(ret->count)))
        ret = NULL;

    if (ret)
    {
        u8 *buffer = mem_new(ret->sector);

        if (buffer && disk_read(disk, buffer, 0, 0))
        {
            u8 *info = &(buffer[0x1BE + ((partition - 1) * 16)]);
            mem_copy(&(ret->lba),   &(info[sizeof(u32) * 2]), sizeof(u32));
            mem_copy(&(ret->count), &(info[sizeof(u32) * 3]), sizeof(u32));

            ret->disk = disk;
        }
        else
            ret = NULL;

        mem_del(buffer);
    }

    return (dev_disk){.driver = &mbr, .context = ret};
}

extern void
mbr_clean(dev_disk *d)
{
    if (d)
        d->context = NULL;
}
