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

#include <core/block.h>

extern bool
block_stat(dev_block *db, u32 idx, u32 *width, u32 *length)
{
    return db->driver->stat(db->context, idx, width, length);
}

extern bool
block_read(dev_block *db, u32 idx, void *buf, u32 block)
{
    return db->driver->read(db->context, idx, buf, block);
}

extern bool
block_write(dev_block *db, u32 idx, void *buf, u32 block)
{
    return db->driver->write(db->context, idx, buf, block);
}
