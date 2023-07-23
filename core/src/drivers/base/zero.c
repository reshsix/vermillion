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

#include <_types.h>
#include <string.h>
#include <vermillion/drivers.h>

static bool
stream_read(void *ctx, u8 *buffer)
{
    (void)(ctx);
    *buffer = 0;
    return true;
}

static bool
stream_write(void *ctx, u8 buffer)
{
    (void)(ctx), (void)(buffer);
    return false;
}

static const struct driver zero =
{
    .name = "zero",
    .api = DRIVER_API_STREAM,
    .type = DRIVER_TYPE_GENERIC,
    .interface.stream.read  = stream_read,
    .interface.stream.write = stream_write
};
driver_register(zero);
