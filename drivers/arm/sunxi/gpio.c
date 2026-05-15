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

#include <general/mem.h>
#include <general/types.h>

#include <hal/gpio.h>

#define PN_CFG(c, n, i) *(volatile u32 *)(c + (n * 0x24) + (0x4 * i))
#define PN_DAT(c, n)    *(volatile u32 *)(c + (n * 0x24) + 0x10)
#define PN_PUL(c, n, i) *(volatile u32 *)(c + (n * 0x24) + 0x1C + (0x4 * i))

/* Driver definition */

struct gpio
{
    u32 base;
    u8 io_ports;
};

static struct gpio gpios[2] = {0};

static bool
count(void *ctx, u8 *ports, u8 *slots)
{
    struct gpio *gpio = ctx;
    *ports = gpio->io_ports;
    *slots = 32;

    return true;
}

static bool
read(void *ctx, u8 port, u32 *data)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports)
        *data = PN_DAT(gpio->base, port);
    else
        ret = false;

    return ret;
}

static bool
write(void *ctx, u8 port, u32 data)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports)
        PN_DAT(gpio->base, port) = data;
    else
        ret = false;

    return ret;
}

static bool
info(void *ctx, u8 port, u8 slot, u32 *flags)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports && slot < 32)
    {
        u8 reg = slot / 8, pos = slot % 8;
        u8 role = (PN_CFG(gpio->base, port, reg) >> (pos * 4)) & 0x7;
        u8 roles[8] = {GPIO_IN, GPIO_OUT, GPIO_CUSTOM(0), GPIO_CUSTOM(1),
                                          GPIO_CUSTOM(2), GPIO_CUSTOM(3),
                                          GPIO_CUSTOM(4), GPIO_OFF};
        role = roles[role];

        reg = slot / 16, pos = slot % 16;
        u8 pull = (PN_PUL(gpio->base, port, reg) >> (pos * 2)) & 0x3;
        if (pull > GPIO_PULLDOWN)
            pull = 0;

        *flags = role | (pull << 4);
    }
    else
        ret = false;

    return ret;
}

static bool
config(void *ctx, u8 port, u8 slot, u32 flags)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports && slot < 32)
    {
        u8 role = (flags >> 0) & 0xF;
        if (role < GPIO_CUSTOM(4))
        {
            u8 roles[8] = {7, 0, 1, 2, 3, 4, 5, 6};
            role = roles[role];
        }
        else
            ret = false;

        u8 pull = (flags >> 4) & 0x3;
        if (pull > GPIO_PULLDOWN)
            ret = false;

        if (ret)
        {
            u8 reg = slot / 8, pos = slot % 8;
            u32 data = role << (pos * 4);
            u32 mask = 0xF  << (pos * 4);
            PN_CFG(gpio->base, port, reg) &= ~mask;
            PN_CFG(gpio->base, port, reg) |= (data & mask);

            reg = slot / 16, pos = slot % 16;
            data = pull << (pos * 2);
            mask = 0x3  << (pos * 2);
            PN_PUL(gpio->base, port, reg) &= ~mask;
            PN_PUL(gpio->base, port, reg) |= (data & mask);
        }
    }
    else
        ret = false;

    return ret;
}

static const drv_gpio sunxi_gpio =
{
    .count = count,
    .read  = read, .write  = write,
    .info  = info, .config = config
};

/* Device creation */

extern dev_gpio
sunxi_gpio_init(u8 id)
{
    struct gpio *ret = NULL;

    if (id < sizeof(gpios) / sizeof(struct gpio))
    {
        ret = &(gpios[id]);
        switch (id)
        {
            case 0:
                ret->base      = 0x01c20800;
                ret->io_ports  = 7;
                break;
            case 1:
                ret->base      = 0x01f02c00;
                ret->io_ports  = 1;
                break;
        }
    }

    return (dev_gpio){.driver = &sunxi_gpio, .context = ret};
}

extern void
sunxi_gpio_clean(dev_gpio *g)
{
    if (g)
        g->context = NULL;
}
