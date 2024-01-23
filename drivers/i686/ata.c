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

#include <i686/env.h>

#include <general/types.h>
#include <general/mem.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/generic/block.h>

struct ata
{
    u16 port;
    bool slave;
};

static bool
ata_poll(struct ata *ata)
{
    bool ret = true;

    while (ret)
    {
        u8 status = in8(ata->port + 7);
        if ((status & (1 << 0)) || (status & (1 << 5)))
            ret = false;
        if ((status & (1 << 3)) && (status & ~(1 << 7)))
            break;
    }

    return ret;
}

static void
init(void **ctx, u16 port, bool slave)
{
    struct ata *ret = mem_new(sizeof(struct ata));

    if (ret)
    {
        ret->port = port;
        ret->slave = slave;

        out8(ret->port + 6, 0xA0 | ret->slave << 4);
        out8(ret->port + 2, 0x0);
        out8(ret->port + 3, 0x0);
        out8(ret->port + 4, 0x0);
        out8(ret->port + 5, 0x0);
        out8(ret->port + 7, 0xEC);

        if (in8(ret->port + 7))
        {
            while (in8(ret->port + 7) & (1 << 7));
            if (!ata_poll(ret))
                ret = mem_del(ret);
        }

        if (ret)
        {
            for (u16 i = 0; i < 256; i++)
                in16(ret->port + 0);
            *ctx = ret;
        }
        else
            mem_del(ret);
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

    (void)ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            *width = 0x200;
            *depth = 0xFFFFFFFF;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static bool
ata_rw(void *ctx, u32 idx, void *buffer, u32 block, bool write)
{
    bool ret = false;

    struct ata *ata = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = true;

            while (in8(ata->port + 7) & (1 << 7));
            out8(ata->port + 6, 0x40 | ata->slave << 4);
            out8(ata->port + 2, 0x0);
            out8(ata->port + 3, (block >> 24) & 0xFF);
            out8(ata->port + 4, 0x0);
            out8(ata->port + 5, 0x0);
            out8(ata->port + 2, 0x1);
            out8(ata->port + 3, (block >> 0)  & 0xFF);
            out8(ata->port + 4, (block >> 8)  & 0xFF);
            out8(ata->port + 5, (block >> 16) & 0xFF);
            out8(ata->port + 7, 0x24 | write << 4);

            in8(ata->port + 7);
            in8(ata->port + 7);
            in8(ata->port + 7);
            in8(ata->port + 7);

            if (ata_poll(ata))
            {
                if (write)
                {
                    for (u16 i = 0; i < 256; i++)
                    {
                        u16 data = 0;
                        u8 *source = buffer;
                        mem_copy(&data, &(source[i * 2]), sizeof(u16));
                        out16(ata->port + 0, data);
                    }
                    out8(ata->port + 7, 0xE7);
                    while (in8(ata->port + 7) & (1 << 7));
                }
                else
                {
                    for (u16 i = 0; i < 256; i++)
                    {
                        u16 data = in16(ata->port + 0);
                        u8 *dest = buffer;
                        mem_copy(&(dest[i * 2]), &data, sizeof(u16));
                    }
                }
            }
            else
                ret = false;

            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    return ata_rw(ctx, idx, buffer, block, false);
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    return ata_rw(ctx, idx, buffer, block, true);
}

drv_decl (block, i686_ata)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
