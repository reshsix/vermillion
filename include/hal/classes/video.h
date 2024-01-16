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

#pragma once

#include <hal/base/drv.h>
#include <hal/base/dev.h>

drv_typedef (block, video);
dev_typedef (video);

struct video_color
{
    u8 pos;
    u8 size;
};

struct [[gnu::packed]] video_fb
{
    u16 width, height;

    u8 bpp;
    struct video_color red, green, blue;
};

bool video_stat(dev_video *dv, u16 *width, u16 *height);
bool video_read(dev_video *dv, void *data, u16 line);
bool video_write(dev_video *dv, void *data, u16 line);
