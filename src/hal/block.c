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

#include <general/types.h>

#include <hal/block.h>

extern bool
block_stat(dev_block *db, u32 idx, u32 *width, u32 *depth)
{
    bool ret = (db != NULL);

    u32 w = 0, d = 0;
    if (ret)
        ret = db->driver->stat(db->context, idx, &w, &d);

    if (ret)
    {
        if (width)
            *width = w;
        if (depth)
            *depth = d;
    }

    return ret;
}

extern bool
block_read(dev_block *db, u32 idx, void *buf, u32 block)
{
    bool ret = (db != NULL);

    if (ret)
        ret = db->driver->read(db->context, idx, buf, block);

    return ret;
}

extern bool
block_write(dev_block *db, u32 idx, void *buf, u32 block)
{
    bool ret = (db != NULL);

    if (ret)
        ret = db->driver->write(db->context, idx, buf, block);

    return ret;
}
