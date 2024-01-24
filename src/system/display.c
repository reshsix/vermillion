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

#include <hal/classes/video.h>

#include <system/display.h>

struct sheet
{
    u32 *data;
    u8 hc, vc;
    u16 hs, vs;
};
static struct sheet sheet = {0};

struct buffer
{
    u32 *data;
    u32 *scroll;
    u16 hs, vs;
};
static struct buffer buffer = {0};

struct bg
{
    u32 *data;
    u16 hs, vs;
    u8 op;
};
static struct bg bg = {0};

static dev_video *vdev = NULL;

extern bool
display_info(void **background, u16 *w, u16 *h, u8 *op,
             void **spritesheet, u8 *hc, u8 *vc, u16 *hs, u16 *vs)
{
    if (background) *background = bg.data;
    if (w)          *w = bg.hs;
    if (h)          *h = bg.vs;
    if (op)         *op = bg.op;

    if (spritesheet) *spritesheet = sheet.data;
    if (hc)          *hc = sheet.hc;
    if (vc)          *vc = sheet.vc;
    if (hs)          *hs = sheet.hs;
    if (vs)          *vs = sheet.vs;

    return true;
}

extern bool
display_config(void *background, u16 w, u16 h, u8 op,
               void *spritesheet, u8 hc, u8 vc, u16 hs, u16 vs)
{
    bg.data = background;
    bg.hs = w;
    bg.vs = h;
    bg.op = op;

    sheet.data = spritesheet;
    sheet.hc = hc;
    sheet.vc = vc;
    sheet.hs = hs;
    sheet.vs = vs;

    return true;
}

extern bool
display_check(dev_video **video)
{
    if (video) *video = vdev;
    return true;
}

extern bool
display_setup(dev_video *video)
{
    bool ret = video_stat(video, &(buffer.hs), &(buffer.vs));

    if (ret)
    {
        u32 *new = mem_renew(buffer.data, buffer.hs * buffer.vs * 4);
        buffer.data = (new) ? new : mem_del(buffer.data);
        ret = (buffer.data);
    }

    if (ret)
    {
        u32 *new = mem_renew(buffer.scroll, buffer.hs * buffer.vs * 4);
        buffer.scroll = (new) ? new : mem_del(buffer.scroll);
        ret = (buffer.scroll);
    }

    if (ret)
        vdev = video;
    else
    {
        mem_del(buffer.data);
        mem_del(buffer.scroll);
    }

    return ret;
}

static bool
display_clear_opt(u16 xp, u16 yp, u16 wp, u16 hp, bool scroll)
{
    bool ret = (vdev && bg.data && buffer.data);

    if (ret)
    {
        u16 x  = (xp * buffer.hs) / 65535, y = (yp * buffer.vs) / 65535;
        u16 w  = (wp * buffer.hs) / 65535, h = (hp * buffer.vs) / 65535;

        u16 bi = 0, bj = 0, bw = bg.hs, bh = bg.vs;
        u32 br = (bw * 65535) / bh;
        u32 dr = (buffer.hs * 65535) / buffer.vs;

        if (br > dr)
        {
            u16 nw = (bh * dr) / 65535;
            bi += (bw - nw) / 2;
            bw = nw;
        }
        else
        {
            u16 nh = (bw * 65535) / dr;
            bj += (bh - nh) / 2;
            bh = nh;
        }

        for (u16 j = y; ret && j < y + h; j++)
        {
            u16 by = bj + (j * bh) / buffer.vs;
            for (u16 i = x; i < x + w; i++)
            {
                u16 bx = bi + (i * bw) / buffer.hs;

                u8 *src = (u8*)&(bg.data[(by * bg.hs) + bx]);
                u8 *dst = (u8*)&(buffer.data[(j * buffer.hs) + i]);

                u8 a = bg.op;
                dst[0] = (src[0] * a)/255;
                dst[1] = (src[1] * a)/255;
                dst[2] = (src[2] * a)/255;

                if (scroll)
                {
                    u8 *dst2 = (u8*)&(buffer.scroll[(j * buffer.hs) + i]);
                    dst2[0] = 0x0;
                    dst2[1] = 0x0;
                    dst2[2] = 0x0;
                    dst2[3] = 0x0;
                }
            }
            ret = video_write(vdev, &(buffer.data[(j * buffer.hs)]), j);
        }
    }

    return ret;
}

extern bool
display_clear(u16 xp, u16 yp, u16 wp, u16 hp)
{
    return display_clear_opt(xp, yp, wp, hp, true);
}

extern bool
display_vscroll(u16 p, bool up)
{
    bool ret = display_clear_opt(0, 0, 65535, 65535, false);

    if (ret)
    {
        u16 count = (p * buffer.vs) / 65535;
        if (up)
        {
            for (u16 i = 0; i < buffer.vs; i++)
            {
                void *dst = &(buffer.scroll[(i * buffer.hs)]);
                if (i < (buffer.vs - count))
                {
                    void *src = &(buffer.scroll[((i + count) * buffer.hs)]);
                    mem_copy(dst, src, buffer.hs * 4);
                }
                else
                    mem_init(dst, 0x0, buffer.hs * 4);
            }
        }
        else
        {
            for (s32 i = buffer.vs - 1; i >= 0; i--)
            {
                void *dst = &(buffer.scroll[(i * buffer.hs)]);
                if (i < count)
                    mem_init(dst, 0x0, buffer.hs * 4);
                else
                {
                    void *src = &(buffer.scroll[((i - count) * buffer.hs)]);
                    mem_copy(dst, src, buffer.hs * 4);
                }
            }
        }

        for (u16 j = 0; ret && j < buffer.vs; j++)
        {
            for (u16 i = 0; i < buffer.hs; i++)
            {
                u8 *src = (u8*)&(buffer.scroll[(j * buffer.hs) + i]);
                u8 *dst = (u8*)&(buffer.data[(j * buffer.hs) + i]);

                u8 a = src[3];
                dst[0] = ((src[0] * a) + (dst[0] * (255 - a)))/255;
                dst[1] = ((src[1] * a) + (dst[1] * (255 - a)))/255;
                dst[2] = ((src[2] * a) + (dst[2] * (255 - a)))/255;
            }
            ret = video_write(vdev, &(buffer.data[(j * buffer.hs)]), j);
        }
    }

    return ret;
}

extern bool
display_show(u8 n, u16 xp, u16 yp, u16 wp, u16 hp, enum display_align align)
{
    bool ret = (vdev && sheet.data && buffer.data &&
                n < sheet.hc * sheet.vc);
    if (ret)
    {
        u16 x  = (xp * buffer.hs) / 65535, y = (yp * buffer.vs) / 65535;
        u16 w  = (wp * buffer.hs) / 65535, h = (hp * buffer.vs) / 65535;
        u16 hs = sheet.hs / sheet.hc,      vs = sheet.vs / sheet.vc;
        u16 oi = (n % sheet.hc) * hs,      oj = (n / sheet.vc) * vs;
        u32 dr = (w * 65535) / h;

        if (align != DISPLAY_STRETCH)
        {
            u32 sr = (hs * 65535) / vs;
            if (sr > dr)
            {
                u16 nh = (w * 65535) / sr;
                switch (align)
                {
                    case DISPLAY_START:                     break;
                    case DISPLAY_MIDDLE: y += (h - nh) / 2; break;
                    case DISPLAY_END:    y += (h - nh);     break;
                    default:
                        break;
                }
                h = nh;
            }
            else if (dr > sr)
            {
                u16 nw = (h * sr) / 65535;
                switch (align)
                {
                    case DISPLAY_START:                     break;
                    case DISPLAY_MIDDLE: x += (w - nw) / 2; break;
                    case DISPLAY_END:    x += (w - nw);     break;
                    default:
                        break;
                }
                w = nw;
            }
        }

        for (u16 j = 0; ret && j < h; j++)
        {
            u16 ry = y + j;
            u16 rj = oj + ((j * vs) / h);
            for (u16 i = 0; i < w; i++)
            {
                u16 rx = x + i;
                u16 ri = oi + ((i * hs) / w);
                if (rx < buffer.hs && ry < buffer.vs)
                {
                    u8 *src = (u8*)&(sheet.data[(rj * sheet.hs) + ri]);
                    u8 *dst = (u8*)&(buffer.data[(ry * buffer.hs) + rx]);

                    u8 a = src[3];
                    dst[0] = ((src[0] * a) + (dst[0] * (255 - a)))/255;
                    dst[1] = ((src[1] * a) + (dst[1] * (255 - a)))/255;
                    dst[2] = ((src[2] * a) + (dst[2] * (255 - a)))/255;

                    u8 *dst2 = (u8*)&(buffer.scroll[(ry * buffer.hs) + rx]);
                    dst2[0] = ((src[0] * a) + (dst2[0] * (255 - a)))/255;
                    dst2[1] = ((src[1] * a) + (dst2[1] * (255 - a)))/255;
                    dst2[2] = ((src[2] * a) + (dst2[2] * (255 - a)))/255;
                    dst2[3] = ((a + dst2[3]) > 255) ? 255 : a + dst2[3];
                }
            }
            ret = video_write(vdev, &(buffer.data[(ry * buffer.hs)]), ry);
        }
    }

    return ret;
}
