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

struct memory
{
    u32 base, width, depth;
};

static void
init(void **ctx, u32 base, u32 width, u32 depth)
{
    struct memory *ret = mem_new(sizeof(struct memory));

    if (ret)
    {
        ret->base = base;
        ret->width = width;
        ret->depth = depth;

        *ctx = ret;
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
        struct memory *mem = ctx;
        switch (idx)
        {
            case 0:
                *width = mem->width;
                *depth = mem->depth;
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
        struct memory *mem = ctx;
        switch (idx)
        {
            case 0:
                ret = (block < mem->depth);

                if (ret)
                    mem_copy(buffer, (void*)(mem->base + (block * mem->width)),
                             mem->width);
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
        struct memory *mem = ctx;
        switch (idx)
        {
            case 0:
                ret = (block < mem->depth);

                if (ret)
                    mem_copy((void*)(mem->base + (block * mem->width)), buffer,
                             mem->width);
                break;
        }
    }

    return ret;
}

drv_decl (block, memory)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
