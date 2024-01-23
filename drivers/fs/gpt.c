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

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/generic/block.h>

struct gpt
{
    dev_block *storage;
    u32 lba, width, depth;
};

static void
init(void **ctx, dev_block *storage, u8 partition)
{
    struct gpt *ret = NULL;

    if (storage && partition > 0 && partition < 128)
        ret = mem_new(sizeof(struct gpt));

    if (ret && !block_stat(storage, BLOCK_COMMON, &(ret->width), NULL))
        ret = mem_del(ret);

    if (ret)
    {
        u8 *buffer = mem_new(ret->width);

        u32 entries = 0, entry_s = 0;
        if (buffer && block_read(storage, BLOCK_COMMON, buffer, 1))
        {
            mem_copy(&entries, &(buffer[0x48]), sizeof(u64));
            mem_copy(&entry_s, &(buffer[0x54]), sizeof(u32));
            u32 lba = entries + ((partition * entry_s) / ret->width);
            u32 pos = (partition * entry_s) % ret->width;

            u64 start = 0, end = 0;
            if (block_read(storage, BLOCK_COMMON, buffer, lba))
            {
                mem_copy(&start, &(buffer[pos + 0x20]), sizeof(u32));
                mem_copy(&end,   &(buffer[pos + 0x28]), sizeof(u32));
                if (start <= 0xFFFFFFFF && end <= 0xFFFFFFFF)
                {
                    ret->lba = start;
                    ret->depth = end - start;
                }
            }
            ret->storage = storage;
        }

        if (ret->depth != 0)
            *ctx = ret;
        else
            ret = mem_del(ret);

        mem_del(buffer);
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

    struct gpt *gpt = ctx;
    switch (idx)
    {
        case 0:
            *width = gpt->width;
            *depth = gpt->depth;
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

    struct gpt *gpt = ctx;
    switch (idx)
    {
        case 0:
            ret = (block < gpt->depth);

            if (ret)
                ret = block_read(gpt->storage, BLOCK_COMMON,
                                 buffer, block + gpt->lba);
        break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct gpt *gpt = ctx;
    switch (idx)
    {
        case 0:
            ret = (block < gpt->depth);

            if (ret)
                ret = block_write(gpt->storage, BLOCK_COMMON,
                                  buffer, block + gpt->lba);
        break;
    }

    return ret;
}

drv_decl (block, gpt)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
