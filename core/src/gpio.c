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

#include <core/gpio.h>
#include <core/block.h>

extern bool
gpio_stat(dev_gpio *dg, u32 *width, u32 *ports)
{
    return dg->driver->stat(dg->context, 0, width, ports);
}

extern bool
gpio_read(dev_gpio *dg, u16 port, void *data)
{
    return dg->driver->read(dg->context, 0, data, port);
}

extern bool
gpio_write(dev_gpio *dg, u16 port, void *data)
{
    return dg->driver->write(dg->context, 0, data, port);
}

extern bool
gpio_get(dev_gpio *dg, u16 pin, bool *data)
{
    return dg->driver->read(dg->context, 1, data, pin);
}

extern bool
gpio_set(dev_gpio *dg, u16 pin, bool data)
{
    return dg->driver->write(dg->context, 1, &data, pin);
}

extern bool
gpio_info(dev_gpio *dg, u16 id, enum gpio_role *role, enum gpio_pull *pull)
{
    struct gpio_pin pin = {0};

    bool ret = dg->driver->read(dg->context, 2, &pin, id);

    if (ret)
    {
        if (role)
            *role = pin.role;
        if (pull)
            *pull = pin.pull;
    }

    return ret;
}

extern bool
gpio_config(dev_gpio *dg, u16 id, enum gpio_role role, enum gpio_pull pull)
{
    struct gpio_pin pin = {.role = role, .pull = pull};
    return dg->driver->write(dg->context, 2, &pin, id);
}

extern bool
gpio_check(dev_gpio *dg, u16 id, bool *enabled,
           void (**handler)(void *), void **arg, enum gpio_level *level)
{
    struct gpio_intr intr = {0};

    bool ret = dg->driver->read(dg->context, 3, &intr, id);

    if (ret)
    {
        if (enabled)
            *enabled = intr.enabled;
        if (handler)
            *handler = intr.handler;
        if (arg)
            *arg = intr.arg;
        if (level)
            *level = intr.level;
    }

    return ret;
}

extern bool
gpio_setup(dev_gpio *dg, u16 id, bool enabled,
           void (*handler)(void *), void *arg, enum gpio_level level)
{
    struct gpio_intr intr = {.enabled = enabled, .handler = handler,
                             .arg = arg, .level = level};
    return dg->driver->write(dg->context, 3, &intr, id);
}
