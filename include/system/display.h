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

#include <general/types.h>

#include <hal/classes/video.h>

bool display_info(void **background, u16 *w, u16 *h, u8 *op,
                  void **spritesheet, u8 *hc, u8 *vc, u16 *hs, u16 *vs);
bool display_config(void *background, u16 w, u16 h, u8 op,
                    void *spritesheet, u8 hc, u8 vc, u16 hs, u16 vs);

bool display_check(dev_video **video);
bool display_setup(dev_video *video);

enum display_align
{
    DISPLAY_STRETCH,
    DISPLAY_START,
    DISPLAY_MIDDLE,
    DISPLAY_END
};

bool display_clear(u16 xp, u16 yp, u16 wp, u16 hp);
bool display_vscroll(u16 p, bool up);
bool display_show(u8 n, u16 xp, u16 yp, u16 wp, u16 hp,
                  enum display_align align);
