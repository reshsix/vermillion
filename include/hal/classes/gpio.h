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

#include <hal/block.h>

enum gpio_index
{
    GPIO_PINS = BLOCK_COMMON + 1,
    GPIO_CONFIG_PIN,
    GPIO_CONFIG_EINT
};

typedef drv_block drv_gpio;
typedef dev_block dev_gpio;

enum gpio_role
{
    GPIO_OFF, GPIO_IN, GPIO_OUT, GPIO_EINT, GPIO_CUSTOM
};

enum gpio_pull
{
    GPIO_PULLOFF, GPIO_PULLUP, GPIO_PULLDOWN
};

enum gpio_level
{
    GPIO_EDGE_H, GPIO_EDGE_L, GPIO_LEVEL_H, GPIO_LEVEL_L, GPIO_DOUBLE
};

struct gpio_pin
{
    enum gpio_role role;
    enum gpio_pull pull;
};

struct gpio_intr
{
    bool enabled;
    void (*handler)(void *), *arg;
    enum gpio_level level;
};

bool gpio_stat(dev_gpio *dg, u32 *width, u32 *ports);
bool gpio_read(dev_gpio *dg, u16 port, void *data);
bool gpio_write(dev_gpio *dg, u16 port, void *data);

bool gpio_get(dev_gpio *dg, u16 pin, bool *data);
bool gpio_set(dev_gpio *dg, u16 pin, bool data);

bool gpio_info(dev_gpio *dg, u16 id,
               enum gpio_role *role, enum gpio_pull *pull);
bool gpio_config(dev_gpio *dg, u16 id,
                 enum gpio_role role, enum gpio_pull pull);
bool gpio_check(dev_gpio *dg, u16 id, bool *enabled,
                void (**handler)(void *), void **arg, enum gpio_level *level);
bool gpio_setup(dev_gpio *dg, u16 id, bool enabled,
                void (*handler)(void *), void *arg, enum gpio_level level);
