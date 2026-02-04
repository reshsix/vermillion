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

#pragma once

#include <general/types.h>

enum block_index
{
    BLOCK_COMMON
};

typedef struct [[gnu::packed]]
{
    void *init, (*clean)(void *);
    bool (*stat) (void *ctx, u32 idx, u32 *width, u32 *length);
    bool (*read) (void *ctx, u32 idx, void *buffer, u32 block);
    bool (*write)(void *ctx, u32 idx, void *buffer, u32 block);
} drv_block;

typedef struct [[gnu::packed]]
{
    const drv_block *driver;
    void *context;
} dev_block;

bool block_stat(dev_block *db, u32 idx, u32 *width, u32 *depth);
bool block_read(dev_block *db, u32 idx, void *buffer, u32 block);
bool block_write(dev_block *db, u32 idx, void *buffer, u32 block);
