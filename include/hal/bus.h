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

#pragma once

#include <general/types.h>

typedef struct
{
    void *init, (*clean)(void *);
    bool (*ioctl)(void *ctx, u8 idx, void *data);
    bool (*stat)(void *ctx, size_t *width, size_t *length);
    bool (*transfer)(void *ctx, void *data, size_t count);
    bool (*poll)(void *ctx);
} drv_bus;

typedef struct
{
    const drv_bus *driver;
    void *context;
} dev_bus;

bool bus_ioctl(dev_bus *ds, u8 idx, void *data);
bool bus_stat(dev_bus *ds, size_t *width, size_t *length);
bool bus_transfer(dev_bus *ds, void *data, size_t count);
bool bus_poll(dev_bus *ds);
