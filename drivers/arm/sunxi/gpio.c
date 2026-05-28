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
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

#define PN_CFG(c, n, i) \
    *(volatile uint32_t *)(c + (n * 0x24) + (0x4 * i))
#define PN_DAT(c, n)    \
    *(volatile uint32_t *)(c + (n * 0x24) + 0x10)
#define PN_PUL(c, n, i) \
    *(volatile uint32_t *)(c + (n * 0x24) + 0x1C + (0x4 * i))

/* Driver definition */

struct gpio
{
    uint32_t base;
    uint8_t io_ports;
};

static struct gpio gpios[2] = {0};

static bool
info(void *ctx, uint8_t port, uint8_t slot, uint32_t *flags)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports && slot < 32)
    {
        uint8_t reg = slot / 8, pos = slot % 8;
        uint8_t role = (PN_CFG(gpio->base, port, reg) >> (pos * 4)) & 0x7;
        uint8_t roles[8] = {VRM_GPIO_IN,   VRM_GPIO_OUT,
                            VRM_GPIO_MUX0, VRM_GPIO_MUX1,
                            VRM_GPIO_MUX2, VRM_GPIO_MUX3,
                            VRM_GPIO_MUX4, VRM_GPIO_OFF};
        role = roles[role];

        reg = slot / 16, pos = slot % 16;
        uint8_t pull = (PN_PUL(gpio->base, port, reg) >> (pos * 2)) & 0x3;
        if (pull > VRM_GPIO_PULLDOWN)
            pull = 0;

        *flags = role | (pull << 4);
    }
    else
        ret = false;

    return ret;
}

static bool
config(void *ctx, uint8_t port, uint8_t slot, uint32_t flags)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports && slot < 32)
    {
        uint8_t role = (flags >> 0) & 0xF;
        if (role < VRM_GPIO_MUX4)
        {
            uint8_t roles[8] = {7, 0, 1, 2, 3, 4, 5, 6};
            role = roles[role];
        }
        else
            ret = false;

        uint8_t pull = (flags >> 4) & 0x3;
        if (pull > VRM_GPIO_PULLDOWN)
            ret = false;

        if (ret)
        {
            uint8_t reg = slot / 8, pos = slot % 8;
            uint32_t data = role << (pos * 4);
            uint32_t mask = 0xF  << (pos * 4);
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

static bool
count(void *ctx, uint8_t *ports, uint8_t *slots)
{
    struct gpio *gpio = ctx;
    *ports = gpio->io_ports;
    *slots = 32;

    return true;
}

static bool
read(void *ctx, uint8_t port, uint32_t *data)
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
write(void *ctx, uint8_t port, uint32_t data)
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
get(void *ctx, uint8_t port, uint8_t pin, bool *data)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports && pin < 32)
        *data = (PN_DAT(gpio->base, port) & (1 << pin));
    else
        ret = false;

    return ret;
}

static bool
set(void *ctx, uint8_t port, uint8_t pin, bool data)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (port < gpio->io_ports && pin < 32)
    {
        if (data)
            PN_DAT(gpio->base, port) |=  (1 << pin);
        else
            PN_DAT(gpio->base, port) &= ~(1 << pin);
    }
    else
        ret = false;

    return ret;
}

static const drv_gpio sunxi_gpio =
{
    .info  = info, .config = config,
    .count = count,
    .read  = read, .write  = write,
    .get   = get,  .set    = set
};

/* Device creation */

extern dev_gpio
sunxi_gpio_init(uint8_t id)
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
