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

#include <general/types.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/gpio.h>

/* Devtree setup */

static dev_gpio *dev_l = NULL;
static u8 dev_c = 0;

extern void
gpio_setup(dev_gpio *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define GPIO_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
gpio_info(u8 id, u8 port, u8 slot, u32 *fields)
{
    u32 fields2 = 0;

    bool ret = GPIO_CALL(info, port, slot, &fields2);
    if (ret && fields)
        *fields = fields2;

    return ret;
}

extern bool
gpio_config(u8 id, u8 port, u8 slot, u32 fields)
{
    return GPIO_CALL(config, port, slot, fields);
}

extern bool
gpio_count(u8 id, u8 *ports, u8 *slots)
{
    return GPIO_CALL(count, ports, slots);
}

extern bool
gpio_read(u8 id, u8 port, u32 *data)
{
    return GPIO_CALL(read, port, data);
}

extern bool
gpio_write(u8 id, u8 port, u32 data)
{
    return GPIO_CALL(write, port, data);
}

extern bool
gpio_get(u8 id, u8 port, u8 pin, bool *data)
{
    return GPIO_CALL(get, port, pin, data);
}

extern bool
gpio_set(u8 id, u8 port, u8 pin, bool data)
{
    return GPIO_CALL(set, port, pin, data);
}

/* ABI definitions */

static struct vrm_gpio_v1 v1 =
{
    .info  = gpio_info, .config = gpio_config,
    .count = gpio_count,
    .read  = gpio_read, .write  = gpio_write,
    .get   = gpio_get,  .set    = gpio_set
};

extern void *
gpio_driver(u8 version)
{
    void *ret = NULL;

    switch (version)
    {
        case VRM_GPIO_V1:
            ret = &v1;
            break;
    }

    return ret;
}
