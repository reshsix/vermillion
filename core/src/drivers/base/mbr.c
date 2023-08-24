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

#include <_types.h>
#include <stdlib.h>
#include <string.h>
#include <vermillion/drivers.h>

static u8 mbr_buf[0x200] __attribute__((aligned(32)));

struct mbr
{
    struct device *storage;
    u32 lba;
};

static void
init(void **ctx, struct device *storage, u8 partition)
{
    struct mbr *ret = NULL;

    if (storage && partition > 0 && partition < 5)
        ret = calloc(1, sizeof(struct mbr));

    if (ret && storage->driver->interface.block.read(storage->context,
                                                     mbr_buf, 0))
    {
        ret->storage = storage;
        ret->lba = ((u32*)&(mbr_buf[0x1BE + ((partition - 1) * 16)]))[2];
        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    free(ctx);
}

static bool
block_read(void *ctx, u8 *buffer, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if ((UINT32_MAX) - mbr->lba > block)
        ret = mbr->storage->driver->interface.block.read(mbr->storage->context,
                                                         buffer,
                                                         block + mbr->lba);
    return ret;
}

static bool
block_write(void *ctx, u8 *buffer, u32 block)
{
    bool ret = false;

    struct mbr *mbr = ctx;
    if ((UINT32_MAX) - mbr->lba > block)
        ret = mbr->storage->driver->interface.block.write
                (mbr->storage->context, buffer, block + mbr->lba);

    return ret;
}

DECLARE_DRIVER(mbr)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_BLOCK,
    .type = DRIVER_TYPE_STORAGE,
    .interface.block.read  = block_read,
    .interface.block.write = block_write
};
