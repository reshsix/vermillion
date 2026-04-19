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

#include <hal/bus.h>

extern bool
bus_ioctl(dev_bus *db, u8 idx, void *data)
{
    bool ret = (db != NULL);

    if (ret)
        ret = db->driver->ioctl(db->context, idx, data);

    return ret;
}

extern bool
bus_stat(dev_bus *db, size_t *width, size_t *length)
{
    bool ret = (db != NULL);

    size_t w = 0, l = 0;
    if (ret)
        ret = db->driver->stat(db->context, &w, &l);

    if (ret)
    {
        if (width)
            *width = w;
        if (length)
            *length = l;
    }

    return ret;
}

extern bool
bus_transfer(dev_bus *db, void *data, size_t count)
{
    bool ret = (db != NULL);

    if (ret)
        ret = db->driver->transfer(db->context, data, count);

    return ret;
}

extern bool
bus_poll(dev_bus *db)
{
    bool ret = (db != NULL);

    if (ret)
        ret = db->driver->poll(db->context);

    return ret;
}
