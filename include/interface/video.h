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

#ifndef INTERFACE_VIDEO_H
#define INTERFACE_VIDEO_H

#include <types.h>
#include <drivers/dummy.h>
#include <drivers/ili9488.h>

bool _video_init(void);
void _video_clean(void);

void video_update(u8 *buffer, u16 x, u16 y, u16 w, u16 h);
void video_clear(void);

#endif
