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

#define PN_CFG(c, n, i) *(volatile u32*)(c + (n * 0x24) + (0x4 * i))
#define PN_DAT(c, n)    *(volatile u32*)(c + (n * 0x24) + 0x10)
#define PN_PUL(c, n, i) *(volatile u32*)(c + (n * 0x24) + 0x1C + (0x4 * i))

#define EINT_CFG(c, n, i) *(volatile u32*)(c + 0x200 + (n * 0x20) + (0x4 * i))
#define EINT_CTL(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x10)
#define EINT_STA(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x14)

enum pin_config
{
    PIN_CFG_IN = 0,
    PIN_CFG_OUT,
    PIN_CFG_EX0,
    PIN_CFG_EX1,
    PIN_CFG_EX2,
    PIN_CFG_EX3,
    PIN_CFG_EX4,
    PIN_CFG_OFF
};

enum pin_pull
{
    PIN_PULLOFF = 0,
    PIN_PULLUP,
    PIN_PULLDOWN
};

enum eint_config
{
    EINT_EDGE_H,
    EINT_EDGE_L,
    EINT_LEVEL_H,
    EINT_LEVEL_L,
    EINT_DOUBLE
};

struct gpio
{
    u32 base;
    u8 io_ports;
    u8 int_ports;
    union config config;
};

static bool gpio_pin(void *ctx, u16 pin, u8 role, u8 pull);
static bool gpio_intr(void *ctx, u16 intr, bool enable, u8 level);
static bool gpio_ack(void *ctx, u16 intr);

static void
init(void **ctx, u32 base, u8 io_ports, u8 int_ports)
{
    struct gpio *ret = mem_new(sizeof(struct gpio));

    if (ret)
    {
        ret->base = base;
        ret->io_ports = io_ports;
        ret->int_ports = int_ports;

        ret->config.gpio.pin = gpio_pin;
        ret->config.gpio.intr = gpio_intr;
        ret->config.gpio.ack = gpio_ack;

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    mem_del(ctx);
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct gpio *gpio = ctx;
    mem_copy(cfg, &(gpio->config), sizeof(union config));
    return true;
}

static bool
block_read(void *ctx, u32 idx, u8 *buffer, u32 block)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (idx == 0 && block < gpio->io_ports)
    {
        u32 data = PN_DAT(gpio->base, block);
        mem_copy(buffer, &data, sizeof(u32));
    }
    else
        ret = false;

    return ret;
}

static bool
block_write(void *ctx, u32 idx, u8 *buffer, u32 block)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    if (idx == 0 && block < gpio->io_ports)
    {
        u32 data = 0;
        mem_copy(&data, buffer, sizeof(u32));
        PN_DAT(gpio->base, block) = data;
    }
    else
        ret = false;

    return ret;
}

static bool
gpio_pin(void *ctx, u16 pin, u8 role, u8 pull)
{
    bool ret = false;

    struct gpio *gpio = ctx;

    u8 port = pin / 32, slot = pin % 32;
    if (port < gpio->io_ports)
        ret = true;

    enum pin_config c = PIN_CFG_OFF;
    enum pin_pull pl = PIN_PULLOFF;
    if (ret)
    {
        switch (role)
        {
            case DRIVER_GPIO_OFF:
                break;
            case DRIVER_GPIO_IN:
                c = PIN_CFG_IN;
                break;
            case DRIVER_GPIO_OUT:
                c = PIN_CFG_OUT;
                break;
            default:
                c = role - DRIVER_GPIO_EXTRA;
                break;
        }

        switch (pull)
        {
            case DRIVER_GPIO_PULLOFF:
                break;
            case DRIVER_GPIO_PULLUP:
                pl = PIN_PULLUP;
                break;
            case DRIVER_GPIO_PULLDOWN:
                pl = PIN_PULLDOWN;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
    {
        u8 reg, pos;
        u32 data, mask;

        reg = slot / 8;
        pos = slot % 8;
        data = c << (pos * 4);
        mask = 0xf << (pos * 4);

        PN_CFG(gpio->base, port, reg) &= ~mask;
        PN_CFG(gpio->base, port, reg) |= (data & mask);

        reg = slot / 16;
        pos = slot % 16;
        data = pl << (pos * 2);
        mask = 0x3 << (pos * 2);

        PN_PUL(gpio->base, port, reg) &= ~mask;
        PN_PUL(gpio->base, port, reg) |= (data & mask);
    }

    return ret;
}

static bool
gpio_intr(void *ctx, u16 intr, bool enable, u8 level)
{
    bool ret = false;

    struct gpio *gpio = ctx;

    u8 port = intr / 32, slot = intr % 32;
    if (port < gpio->int_ports)
        ret = true;

    enum eint_config c = EINT_EDGE_H;
    if (ret)
    {
        switch (level)
        {
            case DRIVER_GPIO_EDGE_H:
                break;
            case DRIVER_GPIO_EDGE_L:
                c = EINT_EDGE_L;
                break;
            case DRIVER_GPIO_LEVEL_H:
                c = EINT_LEVEL_H;
                break;
            case DRIVER_GPIO_LEVEL_L:
                c = EINT_LEVEL_L;
                break;
            case DRIVER_GPIO_DOUBLE:
                c = EINT_DOUBLE;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
    {
        if (enable)
            EINT_CTL(gpio->base, port) |= 0x1 << slot;
        else
            EINT_CTL(gpio->base, port) &= ~(0x1 << slot);

        u8 reg = slot / 8;
        u8 pos = slot % 8;

        u32 data = c << (pos * 4);
        u32 mask = 0xf << (pos * 4);

        EINT_CFG(gpio->base, port, reg) &= ~mask;
        EINT_CFG(gpio->base, port, reg) |= (data & mask);
    }

    return ret;
}

static bool
gpio_ack(void *ctx, u16 intr)
{
    bool ret = true;

    u8 port = intr / 32;
    u8 slot = intr % 32;

    struct gpio *gpio = ctx;
    if (port < gpio->int_ports)
        EINT_STA(gpio->base, port) |= 0x1 << slot;
    else
        ret = false;

    return ret;
}

DECLARE_DRIVER(sunxi_gpio)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_BLOCK,
    .type = DRIVER_TYPE_GPIO,
    .config.get = config_get,
    .block.read = block_read,
    .block.write = block_write
};
