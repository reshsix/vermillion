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
#include <general/mem.h>

#include <hal/block.h>

/* Driver definition */

struct mbr
{
    dev_block *storage;
    u32 lba;

    u32 width, depth;
};

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *depth)
{
    bool ret = true;

    struct mbr *mbr = ctx;
    switch (idx)
    {
        case 0:
            *width = mbr->width;
            *depth = mbr->depth;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    switch (idx)
    {
        case 0:
            ret = (block < mbr->depth);

            if (ret)
                ret = block_read(mbr->storage, BLOCK_COMMON,
                                 buffer, block + mbr->lba);
        break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    switch (idx)
    {
        case 0:
            ret = (block < mbr->depth);

            if (ret)
                ret = block_write(mbr->storage, BLOCK_COMMON,
                                  buffer, block + mbr->lba);
        break;
    }

    return ret;
}

static const drv_block mbr =
{
    .stat = stat, .read = read, .write = write
};

/* Device creation */

extern dev_block
mbr_init(dev_block *storage, u8 partition)
{
    struct mbr *ret = NULL;

    if (storage && partition > 0 && partition < 5)
        ret = mem_new(sizeof(struct mbr));

    if (ret && !block_stat(storage, BLOCK_COMMON, &(ret->width), &(ret->depth)))
        ret = mem_del(ret);

    if (ret)
    {
        u8 *buffer = mem_new(ret->width);

        if (buffer && block_read(storage, BLOCK_COMMON, buffer, 0))
        {
            u8 *info = &(buffer[0x1BE + ((partition - 1) * 16)]);
            mem_copy(&(ret->lba),   &(info[sizeof(u32) * 2]), sizeof(u32));
            mem_copy(&(ret->depth), &(info[sizeof(u32) * 3]), sizeof(u32));

            ret->storage = storage;
        }
        else
            ret = mem_del(ret);

        mem_del(buffer);
    }

    return (dev_block){.driver = &mbr, .context = ret};
}

extern void
mbr_clean(dev_block *b)
{
    if (b)
        b->context = mem_del(b->context);
}
