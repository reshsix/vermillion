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

struct virtual
{
    struct video_fb fb;
    u8 depth;
    u8 *buffer;

    dev_video *video;
    u16 width2, height2;
    u8 *buffer2;
};

static void
init(void **ctx, u16 width, u16 height, dev_video *video)
{
    struct virtual *ret = NULL;

    if (width && height && video)
        ret = mem_new(sizeof(struct virtual));

    if (ret)
    {
        if (!(block_read((dev_block *)video, VIDEO_CONFIG, &(ret->fb), 0)))
            ret = mem_del(ret);

        if (ret)
        {
            ret->depth = ret->fb.bpp / 8;

            ret->buffer = mem_new(width * height * ret->depth);
            if (!(ret->buffer))
                ret = mem_del(ret);
        }

        if (ret)
        {
            ret->video = video;
            ret->width2 = ret->fb.width;
            ret->height2 = ret->fb.height;
            ret->fb.width = width;
            ret->fb.height = height;

            u16 w = ret->fb.width, w2 = ret->width2;
            ret->buffer2 = mem_new(w2 * ((w < w2) ? w2 / w : 1) * ret->depth);
            if (!(ret->buffer2))
                ret = mem_del(ret);
        }
    }

    if (ret)
        *ctx = ret;
}

static void
clean(void *ctx)
{
    struct virtual *v = ctx;
    mem_del(v->buffer);
    mem_del(v);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *depth)
{
    bool ret = true;

    struct virtual *v = ctx;
    ret = block_stat((dev_block *)v->video, idx, width, depth);

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct virtual *v = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = (block < v->fb.height);

            if (ret)
                mem_copy(buffer,
                         &(v->buffer[block * v->fb.width * v->depth]),
                         v->fb.width * v->depth);
            break;

        case VIDEO_CONFIG:
            ret = (block == 0);

            if (ret)
                mem_copy(buffer, &(v->fb), sizeof(struct video_fb));
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct virtual *v = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = (block < v->fb.height);

            if (ret)
            {
                mem_copy(&(v->buffer[block * v->fb.width * v->depth]),
                         buffer, v->fb.width * v->depth);

                u16 js = (block * v->height2) / v->fb.height;
                u16 je = js + v->height2 / v->fb.height;
                for (u16 j = js; ret && j < je + 1; j++)
                {
                    for (u16 i = 0; i < v->width2; i++)
                    {
                        u16 i0 = (i * v->fb.width) / v->width2;
                        u16 j0 = (j * v->fb.height) / v->height2;

                        if (j0 > block)
                            break;

                        size_t k1 = ((j0 * v->fb.width) + i0) * v->depth;
                        size_t k2 = (((j - js) * v->width2) + i) * v->depth;

                        u8 *src = &(v->buffer[k1]);
                        u8 *dest = &(v->buffer2[k2]);

                        mem_copy(dest, src, v->depth);
                    }

                    ret = block_write((dev_block *)v->video, BLOCK_COMMON,
                                      v->buffer2, j);
                }
            }
            break;
    }

    return ret;
}

drv_decl (video, virtual_fb)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
