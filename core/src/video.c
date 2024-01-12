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

#include <core/mem.h>
#include <core/dev.h>

#include <core/block.h>
#include <core/video.h>

static dev_video *cache_dv = NULL;
static struct video_fb cache_fb = {0};
static u32 cache_pitch = 0;
static u32 cache_depth = 0;

static u8 *buffer = NULL;
static u32 buffer_s = 0;

extern bool
video_stat(dev_video *dv, u16 *width, u16 *height)
{
    bool ret = block_read((dev_block *)dv, 0, &cache_fb, 0);

    if (ret)
    {
        cache_dv = dv;

        if (width)
            *width = cache_fb.width;
        if (height)
            *height = cache_fb.height;
    }

    return ret;
}

static bool
video_cache(dev_video *dv)
{
    bool ret = video_stat(dv, NULL, NULL);

    if (ret)
        ret = (cache_fb.bpp <= 32) && (cache_fb.bpp % 8 == 0);
    if (ret)
    {
        ret = block_stat((dev_block *)dv, 1, &cache_pitch, NULL);
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

    if (!ret)
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
        ret = block_read((dev_block *)dv, 1, buffer, line);

    if (ret)
    {
        for (u16 i = 0; i < cache_fb.width; i++)
        {
            u32 pixel = 0;
            u32 depth = (cache_depth <= 4) ? cache_depth : sizeof(u32);
            mem_copy(&pixel, &(buffer[i * cache_depth]), depth);

            u32 mask = (1 << cache_fb.bpp) - 1;
            pixel &= mask;

            u32 red_mask   = (1 << cache_fb.red.size)   - 1;
            u32 green_mask = (1 << cache_fb.green.size) - 1;
            u32 blue_mask  = (1 << cache_fb.blue.size)  - 1;

            u32 red    = (pixel >> cache_fb.red.pos)   & red_mask;
            u32 green  = (pixel >> cache_fb.green.pos) & green_mask;
            u32 blue   = (pixel >> cache_fb.blue.pos)  & blue_mask;

            u8 red8   = (red_mask)   ? ((red * 255)   / red_mask)   : 0;
            u8 green8 = (green_mask) ? ((green * 255) / green_mask) : 0;
            u8 blue8  = (blue_mask)  ? ((blue * 255)  / blue_mask)  : 0;
            u8 unused = 0x0;

            u8 *target = data;
            mem_copy(&(target[(i * 4) + 0]), &red8,   sizeof(u8));
            mem_copy(&(target[(i * 4) + 1]), &green8, sizeof(u8));
            mem_copy(&(target[(i * 4) + 2]), &blue8,  sizeof(u8));
            mem_copy(&(target[(i * 4) + 3]), &unused, sizeof(u8));
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
        for (u16 i = 0; i < cache_fb.width; i++)
        {
            u8 *source = data;

            u8 pixel[4] = {0};
            mem_copy(&pixel, &(source[i * sizeof(u32)]), sizeof(u32));

            u32 red_mask   = (1 << cache_fb.red.size)   - 1;
            u32 green_mask = (1 << cache_fb.green.size) - 1;
            u32 blue_mask  = (1 << cache_fb.blue.size)  - 1;

            u32 red   = ((pixel[0] * red_mask)   / 255) << cache_fb.red.pos;
            u32 green = ((pixel[1] * green_mask) / 255) << cache_fb.green.pos;
            u32 blue  = ((pixel[2] * blue_mask)  / 255) << cache_fb.blue.pos;
            u32 result = red | green | blue;

            u8 array[4] = {0};
            array[0] = result & 0xFF;
            array[1] = (result >> 8) & 0xFF;
            array[2] = (result >> 16) & 0xFF;
            array[3] = (result >> 24) & 0xFF;

            u32 depth = (cache_depth <= 4) ? cache_depth : sizeof(u32);
            mem_copy(&(buffer[i * cache_depth]), array, depth);
        }
    }

    if (ret)
        ret = block_write((dev_block *)dv, 1, buffer, line);

    return ret;
}
