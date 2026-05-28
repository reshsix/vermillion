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

#define VERMILLION_INTERNALS
#include <vermillion/hal/gpio.h>
#include <vermillion/util/types.h>

/* Devtree setup */

static dev_gpio *dev_l = NULL;
static uint8_t dev_c = 0;

extern void
gpio_setup(dev_gpio *list, uint8_t count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define GPIO_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
vrm_gpio_info(uint8_t id, uint8_t port, uint8_t slot, uint32_t *fields)
{
    uint32_t fields2 = 0;

    bool ret = GPIO_CALL(info, port, slot, &fields2);
    if (ret && fields)
        *fields = fields2;

    return ret;
}

extern bool
vrm_gpio_config(uint8_t id, uint8_t port, uint8_t slot, uint32_t fields)
{
    return GPIO_CALL(config, port, slot, fields);
}

extern bool
vrm_gpio_count(uint8_t id, uint8_t *ports, uint8_t *slots)
{
    return GPIO_CALL(count, ports, slots);
}

extern bool
vrm_gpio_read(uint8_t id, uint8_t port, uint32_t *data)
{
    return GPIO_CALL(read, port, data);
}

extern bool
vrm_gpio_write(uint8_t id, uint8_t port, uint32_t data)
{
    return GPIO_CALL(write, port, data);
}

extern bool
vrm_gpio_get(uint8_t id, uint8_t port, uint8_t pin, bool *data)
{
    return GPIO_CALL(get, port, pin, data);
}

extern bool
vrm_gpio_set(uint8_t id, uint8_t port, uint8_t pin, bool data)
{
    return GPIO_CALL(set, port, pin, data);
}
