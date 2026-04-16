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

#include <general/types.h>

#include <hal/stream.h>

extern bool
stream_ioctl(dev_stream *ds, u8 idx, void *data)
{
    bool ret = (ds != NULL);

    if (ret)
        ret = ds->driver->ioctl(ds->context, idx, data);

    return ret;
}

extern bool
stream_stat(dev_stream *ds, size_t *width)
{
    bool ret = (ds != NULL);

    size_t w = 0;
    if (ret)
        ret = ds->driver->stat(ds->context, &w);

    if (ret && width)
        *width = w;

    return ret;
}

extern bool
stream_read(dev_stream *ds, void *data)
{
    bool ret = (ds != NULL);

    if (ret)
        ret = ds->driver->read(ds->context, data);

    return ret;
}

extern bool
stream_write(dev_stream *ds, void *data)
{
    bool ret = (ds != NULL);

    if (ret)
        ret = ds->driver->write(ds->context, data);

    return ret;
}
