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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>

static u8 mbr_buf[0x200] __attribute__((aligned(32)));

struct mbr
{
    dev_storage *storage;
    u32 lba;
};

static void
init(void **ctx, dev_storage *storage, u8 partition)
{
    struct mbr *ret = NULL;

    if (storage && partition > 0 && partition < 5)
        ret = mem_new(sizeof(struct mbr));

    if (ret && BLOCK_R(*(storage), 0, mbr_buf, 0))
    {
        ret->storage = storage;
        ret->lba = ((u32*)&(mbr_buf[0x1BE + ((partition - 1) * 16)]))[2];
        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    mem_del(ctx);
}

static bool
block_read(void *ctx, u32 idx, u8 *buffer, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if (idx == 0 && (UINT32_MAX) - mbr->lba > block)
        ret = BLOCK_R(*(mbr->storage), 0, buffer, block + mbr->lba);

    return ret;
}

static bool
block_write(void *ctx, u32 idx, u8 *buffer, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if (idx == 0 && (UINT32_MAX) - mbr->lba > block)
        ret = BLOCK_W(*(mbr->storage), 0, buffer, block + mbr->lba);

    return ret;
}

DECLARE_DRIVER(storage, mbr)
{
    .init = init, .clean = clean,
    .block.read  = block_read,
    .block.write = block_write
};
