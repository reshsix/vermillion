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
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

/* Driver definition */

struct mbr
{
    uint8_t disk;
    uint32_t lba;

    uint16_t sector;
    uint32_t count;
};

static struct mbr mbrs[1] = {0};

static bool
size(void *ctx, uint16_t *sector, uint32_t *count)
{
    struct mbr *mbr = ctx;
    *sector = mbr->sector;
    *count  = mbr->count;

    return true;
}

static bool
read(void *ctx, uint8_t *data, uint32_t block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if (block < mbr->count)
        ret = vrm_disk_read(mbr->disk, data, block + mbr->lba, 0);

    return ret;
}

static bool
write(void *ctx, uint8_t *data, uint32_t block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if (block < mbr->count)
        ret = vrm_disk_write(mbr->disk, data, block + mbr->lba, 0);

    return ret;
}

static const drv_disk mbr =
{
    .size = size, .read = read, .write = write
};

/* Device creation */

extern dev_disk
mbr_init(uint8_t id, uint8_t disk, uint8_t partition)
{
    struct mbr *ret = NULL;

    if (id < (sizeof(mbrs) / sizeof(struct mbr)) &&
        partition > 0 && partition < 5)
        ret = &(mbrs[id]);

    if (ret && !vrm_disk_size(disk, &(ret->sector), &(ret->count)))
        ret = NULL;

    if (ret)
    {
        uint8_t *buffer = mem_new(ret->sector);

        if (buffer && vrm_disk_read(disk, buffer, 0, 0))
        {
            uint8_t *info = &(buffer[0x1BE + ((partition - 1) * 16)]);
            vrm_mem_copy(&(ret->lba),
                         &(info[sizeof(uint32_t) * 2]), sizeof(uint32_t));
            vrm_mem_copy(&(ret->count),
                         &(info[sizeof(uint32_t) * 3]), sizeof(uint32_t));

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
