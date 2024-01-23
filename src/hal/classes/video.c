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

#include <hal/generic/block.h>
#include <hal/classes/video.h>

static dev_video *cache_dv = NULL;
static struct video_fb cache_fb = {0};
static u32 cache_pitch = 0;
static u32 cache_depth = 0;
static bool cache_optimal = false;

static u8 *buffer = NULL;
static u32 buffer_s = 0;

extern bool
video_stat(dev_video *dv, u16 *width, u16 *height)
{
    struct video_fb fb = {0};
    bool ret = block_read((dev_block *)dv, VIDEO_CONFIG, &fb, 0);

    if (ret)
    {
        if (width)
            *width = fb.width;
        if (height)
            *height = fb.height;
    }

    return ret;
}

static bool
video_cache(dev_video *dv)
{
    bool ret = block_read((dev_block *)dv, VIDEO_CONFIG, &cache_fb, 0);

    if (ret)
        ret = (cache_fb.bpp <= 32) && (cache_fb.bpp % 8 == 0);
    if (ret)
    {
        ret = block_stat((dev_block *)dv, BLOCK_COMMON, &cache_pitch, NULL);
        cache_depth = cache_pitch / cache_fb.width;
    }
    if (ret)
        ret = (cache_depth <= sizeof(u32)) && (cache_depth != 0);
    if (ret)
    {
        if (cache_pitch > buffer_s)
        {
            void *new = mem_renew(buffer, cache_pitch);
            if (new)
            {
                buffer = new;
                buffer_s = cache_pitch;
            }
            else
                ret = false;
        }
    }

    if (ret)
    {
        if (cache_depth         == 4 &&
            cache_fb.red.size   == 8 && cache_fb.red.pos   == 16 &&
            cache_fb.green.size == 8 && cache_fb.green.pos == 8  &&
            cache_fb.blue.size  == 8 && cache_fb.blue.pos  == 0)
            cache_optimal = true;

        cache_dv = dv;
    }
    else
        cache_dv = NULL;

    return ret;
}

extern bool
video_read(dev_video *dv, void *data, u16 line)
{
    bool ret = true;

    if (cache_dv != dv)
        ret = video_cache(dv);

    if (ret)
    {
        if (cache_optimal)
            ret = block_read((dev_block *)dv, BLOCK_COMMON, data, line);
        else
        {
            ret = block_read((dev_block *)dv, BLOCK_COMMON, buffer, line);

            const u32 rmask = (1 << cache_fb.red.size)   - 1;
            const u32 gmask = (1 << cache_fb.green.size) - 1;
            const u32 bmask = (1 << cache_fb.blue.size)  - 1;
            for (u16 i = 0; i < cache_fb.width; i++)
            {
                u32 pixel = 0;
                u32 depth = (cache_depth <= 4) ? cache_depth : sizeof(u32);
                mem_copy(&pixel, &(buffer[i * cache_depth]), depth);

                u32 mask = (1 << cache_fb.bpp) - 1;
                pixel &= mask;

                u32 red    = (pixel >> cache_fb.red.pos)   & rmask;
                u32 green  = (pixel >> cache_fb.green.pos) & gmask;
                u32 blue   = (pixel >> cache_fb.blue.pos)  & bmask;

                u8 red8   = (rmask) ? ((red * 255)   / rmask) : 0;
                u8 green8 = (gmask) ? ((green * 255) / gmask) : 0;
                u8 blue8  = (bmask) ? ((blue * 255)  / bmask) : 0;

                u8 *target = data;
                u32 result = red8 << 16 | green8 << 8 | blue8 << 0;
                mem_copy(&(target[i * 4]), &result, cache_depth);
            }
        }
    }

    return ret;
}

extern bool
video_write(dev_video *dv, void *data, u16 line)
{
    bool ret = true;

    if (cache_dv != dv)
        ret = video_cache(dv);

    if (ret)
    {
        if (cache_optimal)
            ret = block_write((dev_block *)dv, BLOCK_COMMON, data, line);
        else
        {
            const u32 rmask = (1 << cache_fb.red.size)   - 1;
            const u32 gmask = (1 << cache_fb.green.size) - 1;
            const u32 bmask = (1 << cache_fb.blue.size)  - 1;

            u8 pixel[4] = {0};
            for (u16 i = 0; i < cache_fb.width; i++)
            {
                u8 *source = data;
                mem_copy(pixel, &(source[i * sizeof(u32)]), sizeof(u32));
                u32 red   = ((pixel[0] * rmask) / 255) << cache_fb.red.pos;
                u32 green = ((pixel[1] * gmask) / 255) << cache_fb.green.pos;
                u32 blue  = ((pixel[2] * bmask) / 255) << cache_fb.blue.pos;
                u32 result = red | green | blue;
                mem_copy(&(buffer[i * cache_depth]), &result, cache_depth);
            }
            ret = block_write((dev_block *)dv, BLOCK_COMMON, buffer, line);
        }
    }

    return ret;
}
