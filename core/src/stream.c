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

#include <core/dev.h>

#include <core/stream.h>

extern bool
stream_stat(dev_stream *ds, u32 idx, u32 *width)
{
    return ds->driver->stat(ds->context, idx, width);
}

extern bool
stream_read(dev_stream *ds, u32 idx, void *data)
{
    return ds->driver->read(ds->context, idx, data);
}

extern bool
stream_write(dev_stream *ds, u32 idx, void *data)
{
    return ds->driver->write(ds->context, idx, data);
}
