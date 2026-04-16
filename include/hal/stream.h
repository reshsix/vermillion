/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <general/types.h>

typedef struct
{
    void *init, (*clean)(void *);
    bool (*ioctl)(void *ctx, u8 idx, void *data);
    bool (*stat) (void *ctx, size_t *width);
    bool (*read) (void *ctx, void *data);
    bool (*write)(void *ctx, void *data);
} drv_stream;

typedef struct
{
    const drv_stream *driver;
    void *context;
} dev_stream;

bool stream_ioctl(dev_stream *ds, u8 idx, void *data);
bool stream_stat(dev_stream *ds,  size_t *width);
bool stream_read(dev_stream *ds,  void *data);
bool stream_write(dev_stream *ds, void *data);
