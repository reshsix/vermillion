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
#include <hal/classes/video.h>

struct framebuffer
{
    u8 *addr;
    u32 pitch;
    struct video_fb fb;
};

struct framebuffer_mb
{
    u64 addr;
    u32 pitch;
    u32 width;
    u32 height;
    u8 bpp;
    u8 type;
    u16 reserved;
    u8 red_pos;
    u8 red_size;
    u8 green_pos;
    u8 green_size;
    u8 blue_pos;
    u8 blue_size;
};

static void
init(void **ctx, void *multiboot_info)
{
    struct framebuffer *ret = NULL;

    u8 *info = multiboot_info;
    if (info)
        ret = mem_new(sizeof(struct framebuffer));

    if (ret)
    {
        u32 size = 0;
        mem_copy(&size, info, sizeof(u32));

        bool found = false;
        u32 idx = sizeof(u32) * 2;
        while (!found && idx < size)
        {
            u32 type = 0, size = 0;
            mem_copy(&type, &(info[idx]),     sizeof(u32));
            mem_copy(&size, &(info[idx + 4]), sizeof(u32));

            if (type == 8)
            {
                idx += sizeof(u32) * 2;

                struct framebuffer_mb fb = {0};
                mem_copy(&fb, &(info[idx]), sizeof(struct framebuffer_mb));

                if (fb.type == 1)
                {
                    ret->addr = (void *)((u32)fb.addr);
                    ret->pitch = fb.pitch;
                    ret->fb.width = fb.width;
                    ret->fb.height = fb.height;
                    ret->fb.bpp = fb.bpp;

                    ret->fb.red.pos = fb.red_pos;
                    ret->fb.red.size = fb.red_size;
                    ret->fb.green.pos = fb.green_pos;
                    ret->fb.green.size = fb.green_size;
                    ret->fb.blue.pos = fb.blue_pos;
                    ret->fb.blue.size = fb.blue_size;
                }
                else
                    ret = mem_del(ret);

                found = true;
            }
            else
            {
                if (size % 8 != 0)
                    size = ((size / 8) + 1) * 8;
                idx += size;
            }
        }

        if (!found)
            ret = mem_del(ret);
    }

    if (ret)
        *ctx = ret;
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

    struct framebuffer *fb = ctx;
    switch (idx)
    {
        case 0:
            *width = sizeof(struct video_fb);
            *depth = 1;
            break;
        case 1:
            *width = fb->pitch;
            *depth = fb->fb.height;
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

    struct framebuffer *fb = ctx;
    switch (idx)
    {
        case 0:
            ret = (block == 0);

            if (ret)
                mem_copy(buffer, &(fb->fb), sizeof(struct video_fb));
            break;

        case 1:
            ret = (block < fb->fb.height);

            if (ret)
                mem_copy(buffer, &(fb->addr[block * fb->pitch]),
                         fb->pitch);
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct framebuffer *fb = ctx;
    switch (idx)
    {
        case 1:
            ret = (block < fb->fb.height);

            if (ret)
                mem_copy(&(fb->addr[block * fb->pitch]), buffer,
                         fb->pitch);
            break;
    }

    return ret;
}

drv_decl (video, i686_fb)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
