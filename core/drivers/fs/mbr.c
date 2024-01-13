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

#include <core/types.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/mem.h>

#include <core/block.h>

struct mbr
{
    dev_block *storage;
    u32 lba;

    u32 width, depth;
};

static void
init(void **ctx, dev_block *storage, u8 partition)
{
    struct mbr *ret = NULL;

    if (storage && partition > 0 && partition < 5)
        ret = mem_new(sizeof(struct mbr));

    if (ret && !block_stat(storage, 0, &(ret->width), &(ret->depth)))
        ret = mem_del(ret);

    if (ret)
    {
        u8 *buffer = mem_new(ret->width);
        if (buffer && block_read(storage, 0, buffer, 0))
        {
            u8 *info = &(buffer[0x1BE + ((partition - 1) * 16)]);
            mem_copy(&(ret->lba),   &(info[sizeof(u32) * 2]), sizeof(u32));
            mem_copy(&(ret->depth), &(info[sizeof(u32) * 3]), sizeof(u32));
            mem_del(buffer);

            ret->storage = storage;
            *ctx = ret;
        }
        else
            ret = mem_del(ret);
    }
}

static void
clean(void *ctx)
{
    mem_del(ctx);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *depth)
{
    bool ret = true;

    if (ctx)
    {
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
    }
    else
        ret = false;

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    if (ctx)
    {
        struct mbr *mbr = ctx;
        switch (idx)
        {
            case 0:
                ret = (block < mbr->depth);

                if (ret)
                    ret = block_read(mbr->storage, 0,
                                     buffer, block + mbr->lba);
            break;
        }
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    if (ctx)
    {
        struct mbr *mbr = ctx;
        switch (idx)
        {
            case 0:
                ret = (block < mbr->depth);

                if (ret)
                    ret = block_write(mbr->storage, 0,
                                      buffer, block + mbr->lba);
            break;
        }
    }

    return ret;
}

drv_decl (block, mbr)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
