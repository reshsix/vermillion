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

#define VRM_GPIO_OFF       (0 << 0)
#define VRM_GPIO_IN        (1 << 0)
#define VRM_GPIO_OUT       (2 << 0)
#define VRM_GPIO_MUX0      (3 << 0)
#define VRM_GPIO_MUX1      (4 << 0)
#define VRM_GPIO_MUX2      (5 << 0)
#define VRM_GPIO_MUX3      (6 << 0)
#define VRM_GPIO_MUX4      (7 << 0)
#define VRM_GPIO_PULLOFF   (0 << 4)
#define VRM_GPIO_PULLUP    (1 << 4)
#define VRM_GPIO_PULLDOWN  (2 << 4)

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    bool (*info)  (void *ctx, uint8_t port, uint8_t slot, uint32_t *fields);
    bool (*config)(void *ctx, uint8_t port, uint8_t slot, uint32_t  fields);
    bool (*count) (void *ctx, uint8_t *ports, uint8_t *slots);
    bool (*read)  (void *ctx, uint8_t port, uint32_t *data);
    bool (*write) (void *ctx, uint8_t port, uint32_t  data);
    bool (*get)   (void *ctx, uint8_t port, uint8_t pin, bool *data);
    bool (*set)   (void *ctx, uint8_t port, uint8_t pin, bool  data);
} drv_gpio;

typedef struct
{
    const drv_gpio *driver;
    void *context;
} dev_gpio;

/* For internal usage */

void gpio_setup(dev_gpio *list, uint8_t count);

/* For external usage */

bool gpio_info  (uint8_t id, uint8_t port, uint8_t slot, uint32_t *fields);
bool gpio_config(uint8_t id, uint8_t port, uint8_t slot, uint32_t  fields);
bool gpio_count (uint8_t id, uint8_t *ports, uint8_t *slots);
bool gpio_read  (uint8_t id, uint8_t port, uint32_t *data);
bool gpio_write (uint8_t id, uint8_t port, uint32_t  data);
bool gpio_get   (uint8_t id, uint8_t port, uint8_t pin, bool *data);
bool gpio_set   (uint8_t id, uint8_t port, uint8_t pin, bool  data);
#endif

struct vrm_gpio_v1
{
    bool (*info)  (uint8_t id, uint8_t port, uint8_t slot, uint32_t *fields);
    bool (*config)(uint8_t id, uint8_t port, uint8_t slot, uint32_t  fields);
    bool (*count) (uint8_t id, uint8_t *ports, uint8_t *slots);
    bool (*read)  (uint8_t id, uint8_t port, uint32_t *data);
    bool (*write) (uint8_t id, uint8_t port, uint32_t  data);
    bool (*get)   (uint8_t id, uint8_t port, uint8_t pin, bool *data);
    bool (*set)   (uint8_t id, uint8_t port, uint8_t pin, bool  data);
};

enum
{
    VRM_GPIO_V1 = 0
};

void *gpio_driver(uint8_t version);
