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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>

#include <easy/io.h>

static struct device *gpio = NULL;

extern void
io_chip(struct device *chip)
{
    gpio = chip;
}

extern void
io_config(u16 pin, u8 mode)
{
    if (gpio)
    {
        switch (mode)
        {
            case OFF:
                pin_cfg(gpio, pin, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);
                break;
            case INPUT_PD:
                pin_cfg(gpio, pin, DRIVER_GPIO_IN, DRIVER_GPIO_PULLDOWN);
                break;
            case INPUT_PU:
                pin_cfg(gpio, pin, DRIVER_GPIO_IN, DRIVER_GPIO_PULLUP);
                break;
            case OUTPUT:
                pin_cfg(gpio, pin, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
                break;
            case FLOATING:
                pin_cfg(gpio, pin, DRIVER_GPIO_IN, DRIVER_GPIO_PULLOFF);
                break;
            default:
                pin_cfg(gpio, pin, mode - EXTRA + DRIVER_GPIO_EXTRA,
                        DRIVER_GPIO_PULLOFF);
                break;
        }
    }
}

extern bool
io_read(u16 pin)
{
    bool value = false;
    if (gpio)
        pin_get(gpio, pin, &value);
    return value;
}

extern void
io_write(u16 pin, bool value)
{
    if (gpio)
        pin_set(gpio, pin, value);
}
