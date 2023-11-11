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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>

static bool
block_read(void *ctx, u8 *buffer, u32 block)
{
    (void)(ctx);
    mem_copy(buffer, (void*)(block * 0x200), 0x200);
    return true;
}

static bool
block_write(void *ctx, u8 *buffer, u32 block)
{
    (void)(ctx);
    mem_copy((void*)(block * 0x200), buffer, 0x200);
    return true;
}

DECLARE_DRIVER(memory)
{
    .api = DRIVER_API_BLOCK,
    .type = DRIVER_TYPE_STORAGE,
    .interface.block.read  = block_read,
    .interface.block.write = block_write
};
