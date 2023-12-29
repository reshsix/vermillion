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

struct virtual
{
    u16 width, height;
    dev_video *video;
    u8 *buffer;

    u16 width2, height2;
    u8 *buffer2;
};

/* TODO other formats and conversion support */

static void
init(void **ctx, u16 width, u16 height, dev_video *video)
{
    struct virtual *ret = NULL;

    if (width && height && video)
        ret = mem_new(sizeof(struct virtual));

    if (ret)
    {
        ret->width = width;
        ret->height = height;
        ret->video = video;

        ret->buffer = mem_new(width * height * 4);
        if (!(ret->buffer))
            ret = mem_del(ret);
    }

    if (ret)
    {
        union config cfg = {0};
        video->driver->config.get(video->context, &cfg);
        ret->width2 = cfg.video.width;
        ret->height2 = cfg.video.height;

        u16 w = ret->width, w2 = cfg.video.width;
        ret->buffer2 = mem_new(w2 * ((w < w2) ? w2 / w : 1) * 4);
        if (!(ret->buffer2))
            ret = mem_del(ret);
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
config_get(void *ctx, union config *data)
{
    struct virtual *v = ctx;

    data->video.width = v->width;
    data->video.height = v->height;
    data->video.format = DRIVER_VIDEO_FORMAT_RGBX32;

    return true;
}

static bool
block_read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = true;

    struct virtual *v = ctx;
    if (idx == 0 && block < v->height)
        mem_copy(buffer, &(v->buffer[block * v->width * 4]), v->width * 4);
    else
        ret = false;

    return ret;
}

static bool
block_write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = true;

    struct virtual *v = ctx;
    if (idx == 0 && block < v->height)
    {
        mem_copy(&(v->buffer[block * v->width * 4]), buffer, v->width * 4);

        u16 js = (block * v->height2) / v->height;
        u16 je = js + v->height2 / v->height;
        for (u16 j = js; ret && j < je + 1; j++)
        {
            for (u16 i = 0; i < v->width2; i++)
            {
                u16 i0 = (i * v->width) / v->width2;
                u16 j0 = (j * v->height) / v->height2;

                if (j0 > block)
                    break;

                u8 *src = &(v->buffer[((j0 * v->width) + i0) * 4]);
                u8 *dest = &(v->buffer2[(((j - js) * v->width2) + i) * 4]);

                mem_copy(dest, src, 4);
            }
            ret = dev_block_write(v->video, 0, v->buffer2, j);
        }
    }
    else
        ret = false;

    return ret;
}

DECLARE_DRIVER(video, virtual_fb)
{
    .init = init, .clean = clean,
    .config.get = config_get,
    .block.read = block_read,
    .block.write = block_write
};
