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

#ifndef CORE_GPIO_H
#define CORE_GPIO_H

#include <core/drv.h>
#include <core/dev.h>

drv_typedef (block, gpio);
dev_typedef (gpio);

enum gpio_role
{
    GPIO_OFF, GPIO_IN, GPIO_OUT, GPIO_CUSTOM
};

enum gpio_pull
{
    GPIO_PULLOFF, GPIO_PULLUP, GPIO_PULLDOWN
};

enum gpio_level
{
    GPIO_EDGE_H, GPIO_EDGE_L, GPIO_LEVEL_H, GPIO_LEVEL_L, GPIO_DOUBLE
};

struct [[packed]] gpio_pin
{
    enum gpio_role role;
    enum gpio_pull pull;
};

struct [[packed]] gpio_intr
{
    bool enabled;
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
bool gpio_check(dev_gpio *dg, u16 id, bool *enabled, enum gpio_level *level);
bool gpio_setup(dev_gpio *dg, u16 id, bool enabled, enum gpio_level level);

#endif
