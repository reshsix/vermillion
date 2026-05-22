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

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    bool (*size) (void *ctx, uint16_t *sector, uint32_t *count);
    bool (*read) (void *ctx, uint8_t  *data,   uint32_t  block);
    bool (*write)(void *ctx, uint8_t  *data,   uint32_t  block);
} drv_disk;

typedef struct
{
    const drv_disk *driver;
    void *context;
} dev_disk;

void disk_setup(dev_disk *list, uint8_t count);

bool disk_size (uint8_t id, uint16_t *sector, uint32_t *count);
bool disk_read (uint8_t id, uint8_t  *data, uint32_t  block, uint32_t flags);
bool disk_write(uint8_t id, uint8_t  *data, uint32_t  block, uint32_t flags);
#endif

struct vrm_disk_v1
{
    bool (*size) (uint8_t id, uint16_t *sector, uint32_t *count);
    bool (*read) (uint8_t id, uint8_t  *data, uint32_t  block, uint32_t flags);
    bool (*write)(uint8_t id, uint8_t  *data, uint32_t  block, uint32_t flags);
};

enum
{
    VRM_DISK_V1 = 0
};

void *disk_driver(uint8_t version);
