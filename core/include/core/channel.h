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

#ifndef CORE_CHANNEL_H
#define CORE_CHANNEL_H

#include <core/types.h>

typedef struct _channel channel;
channel *channel_new(size_t type, size_t size);
channel *channel_del(channel *ch);
void channel_read(channel *ch, void *data);
void channel_write(channel *ch, void *data);

#endif
