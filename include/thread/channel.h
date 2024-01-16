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

typedef struct channel
{
    size_t type, size, count;
    u8 *buffer;
} channel;

channel *channel_new(size_t type, size_t size);
channel *channel_del(channel *ch);
bool channel_empty(channel *ch);
bool channel_full(channel *ch);
size_t channel_stat(channel *ch);
void channel_read(channel *ch, void *data);
void channel_write(channel *ch, void *data);
