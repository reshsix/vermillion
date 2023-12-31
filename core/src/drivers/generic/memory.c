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
#include <core/drv.h>
#include <core/mem.h>

#include <core/storage.h>

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = (idx == 0);

    (void)(ctx);
    if (ret)
        mem_copy(buffer, (void*)(block * 0x200), 0x200);

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = (idx == 0);

    (void)(ctx);
    if (ret)
        mem_copy((void*)(block * 0x200), buffer, 0x200);

    return ret;
}

drv_decl (storage, memory)
{
    .read = read, .write = write
};
