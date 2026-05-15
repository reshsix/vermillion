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

#include <hal/block.h>

#define GPIO_OFF       (0 << 0)
#define GPIO_IN        (1 << 0)
#define GPIO_OUT       (2 << 0)
#define GPIO_CUSTOM(x) (((x) + 3) << 0)

#define GPIO_PULLOFF  (0 << 4)
#define GPIO_PULLUP   (1 << 4)
#define GPIO_PULLDOWN (2 << 4)

typedef struct
{
    void *init, (*clean)(void *);
    bool (*count)(void *ctx, u8 *ports, u8 *slots);
    bool (*read)(void *ctx, u8 port, u32 *data);
    bool (*write)(void *ctx, u8 port, u32 data);
    bool (*info)(void *ctx, u8 port, u8 slot, u32 *fields);
    bool (*config)(void *ctx, u8 port, u8 slot, u32 fields);
} drv_gpio;

typedef struct
{
    const drv_gpio *driver;
    void *context;
} dev_gpio;

/* For internal usage */

void gpio_setup(dev_gpio *list, u8 count);

/* For external usage */

bool gpio_count(u8 id, u8 *ports, u8 *slots);
bool gpio_read(u8 id, u8 port, u32 *data);
bool gpio_write(u8 id, u8 port, u32 data);
bool gpio_info(u8 id, u8 port, u8 slot, u32 *fields);
bool gpio_config(u8 id, u8 port, u8 slot, u32 fields);
