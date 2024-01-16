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

#include <general/types.h>
#include <general/mem.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/classes/pic.h>
#include <hal/classes/gpio.h>

#define PN_CFG(c, n, i) *(volatile u32 *)(c + (n * 0x24) + (0x4 * i))
#define PN_DAT(c, n)    *(volatile u32 *)(c + (n * 0x24) + 0x10)
#define PN_PUL(c, n, i) *(volatile u32 *)(c + (n * 0x24) + 0x1C + (0x4 * i))

#define EINT_CFG(c, n, i) *(volatile u32 *)(c + 0x200 + (n * 0x20) + (0x4 * i))
#define EINT_CTL(c, n)    *(volatile u32 *)(c + 0x200 + (n * 0x20) + 0x10)
#define EINT_STA(c, n)    *(volatile u32 *)(c + 0x200 + (n * 0x20) + 0x14)

static enum gpio_role readable_role[8] =
    {GPIO_IN, GPIO_OUT, GPIO_CUSTOM + 0, GPIO_CUSTOM + 1, GPIO_CUSTOM + 2,
     GPIO_CUSTOM + 3, GPIO_EINT, GPIO_OFF};
static enum gpio_pull readable_pull[4] =
    {GPIO_PULLOFF, GPIO_PULLUP, GPIO_PULLDOWN, GPIO_PULLOFF};
static enum gpio_level readable_level[8] =
    {GPIO_EDGE_H, GPIO_EDGE_L, GPIO_LEVEL_H, GPIO_LEVEL_L,
     GPIO_DOUBLE, GPIO_EDGE_H, GPIO_EDGE_H, GPIO_EDGE_H};

static u8 writable_role[8] = {7, 0, 1, 6, 2, 3, 4, 5};
static u8 writable_pull[3] = {0, 1, 2};
static u8 writable_level[5] = {0, 1, 2, 3, 4};

struct gpio
{
    u32 base;
    u8 io_ports;
    u8 int_ports;

    dev_pic *pic;
    u16 *irqs;
    void (**handlers)(void *), **args;
};

static void
handler(void *arg)
{
    struct gpio *gpio = arg;

    for (u8 i = 0; i < gpio->int_ports; i++)
    {
        for (u8 j = 0; j < 32; j++)
        {
            if (EINT_STA(gpio->base, i) & (1 << j))
            {
                u16 idx = (i * 32) + j;
                EINT_STA(gpio->base, i) |= (1 << j);

                if (gpio->handlers[idx])
                    gpio->handlers[idx](gpio->args[idx]);
            }
        }
    }
}

static void
init(void **ctx, u32 base, u8 io_ports, u8 int_ports, dev_pic *pic, u16 *irqs)
{
    struct gpio *ret = mem_new(sizeof(struct gpio));

    if (ret && int_ports)
    {
        ret->handlers = mem_new(int_ports * 32 * sizeof(void (*)(void *)));
        ret->args = mem_new(int_ports * 32 * sizeof(void *));
        if (!ret->handlers || !ret->args)
        {
            mem_del(ret->handlers);
            mem_del(ret->args);
            ret = mem_del(ret);
        }
    }

    if (ret)
    {
        ret->base = base;
        ret->io_ports = io_ports;
        ret->int_ports = int_ports;

        ret->pic = pic;
        ret->irqs = irqs;
        for (int i = 0; i < int_ports; i++)
            pic_config(pic, irqs[i], true, handler, ret, PIC_EDGE_L);

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct gpio *gpio = ctx;
    for (u8 i = 0; i < gpio->int_ports; i++)
        pic_config(gpio->pic, gpio->irqs[i], false,
                   NULL, NULL, PIC_EDGE_L);

    mem_del(gpio->handlers);
    mem_del(gpio->args);
    mem_del(gpio);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = true;

    struct gpio *gpio = ctx;
    switch (idx)
    {
        case 0:
            *width = sizeof(u32);
            *length = gpio->io_ports;
            break;
        case 1:
            *width = sizeof(bool);
            *length = gpio->io_ports * 32;
            break;
        case 2:
            *width = sizeof(struct gpio_pin);
            *length = gpio->io_ports * 32;
            break;
        case 3:
            *width = sizeof(struct gpio_intr);
            *length = gpio->int_ports * 32;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct gpio *gpio = ctx;
    switch (idx)
    {
        case 0:
            ret = (block < gpio->io_ports);

            if (ret)
            {
                u32 data = PN_DAT(gpio->base, block);
                mem_copy(buffer, &data, sizeof(u32));
            }
            break;

        case 1:
            ret = ((block / 32) < gpio->io_ports);

            if (ret)
            {
                u8 port = block / 32, slot = block % 32;
                bool data = PN_DAT(gpio->base, port) & (1 << slot);
                mem_copy(buffer, &data, sizeof(bool));
            }
            break;

        case 2:
            ret = ((block / 32) < gpio->io_ports);

            if (ret)
            {
                struct gpio_pin pin = {0};
                u8 port = block / 32, slot = block % 32;

                u8 reg = slot / 8, pos = slot % 8;
                pin.role = PN_CFG(gpio->base, port, reg) >> (pos * 4);
                pin.role = readable_role[pin.role % 8];

                reg = slot / 16, pos = slot % 16;
                pin.pull = PN_PUL(gpio->base, port, reg) >> (pos * 2);
                pin.pull = readable_pull[pin.pull % 4];

                mem_copy(buffer, &pin, sizeof(struct gpio_pin));
            }
            break;

        case 3:
            ret = ((block / 32) < gpio->int_ports);

            if (ret)
            {
                struct gpio_intr intr = {0};
                u8 port = block / 32, slot = block % 32;

                intr.enabled = EINT_CTL(gpio->base, port) & (1 << slot);

                u8 reg = slot / 8, pos = slot % 8;
                intr.level = EINT_CFG(gpio->base, port, reg) >> (pos * 4);
                intr.level = readable_level[intr.level % 8];

                intr.handler = gpio->handlers[block];
                intr.arg = gpio->args[block];

                mem_copy(buffer, &intr, sizeof(struct gpio_intr));
            }
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct gpio *gpio = ctx;
    switch (idx)
    {
        case 0:
            ret = (block < gpio->io_ports);

            if (ret)
            {
                u32 data = 0;
                mem_copy(&data, buffer, sizeof(u32));
                PN_DAT(gpio->base, block) = data;
            }
            break;

        case 1:
            ret = ((block / 32) < gpio->io_ports);

            if (ret)
            {
                u8 port = block / 32, slot = block % 32;

                bool data = false;
                mem_copy(&data, buffer, sizeof(bool));

                if (data)
                    PN_DAT(gpio->base, port) |= (1 << slot);
                else
                    PN_DAT(gpio->base, port) &= ~(1 << slot);
            }
            break;

        case 2:
            ret = ((block / 32) < gpio->io_ports);

            struct gpio_pin pin = {0};
            if (ret)
            {
                mem_copy(&pin, buffer, sizeof(struct gpio_pin));
                ret = pin.role < sizeof(writable_role) &&
                      pin.pull < sizeof(writable_pull);
            }

            if (ret)
            {
                u8 port = block / 32, slot = block % 32;

                u8 reg = slot / 8, pos = slot % 8;
                u32 data = writable_role[pin.role] << (pos * 4);
                u32 mask = 0xf << (pos * 4);
                PN_CFG(gpio->base, port, reg) &= ~mask;
                PN_CFG(gpio->base, port, reg) |= (data & mask);

                reg = slot / 16, pos = slot % 16;
                data = writable_pull[pin.pull] << (pos * 2);
                mask = 0x3 << (pos * 2);
                PN_PUL(gpio->base, port, reg) &= ~mask;
                PN_PUL(gpio->base, port, reg) |= (data & mask);
            }
            break;

        case 3:
            ret = ((block / 32) < gpio->int_ports);

            struct gpio_intr intr = {0};
            if (ret)
            {
                mem_copy(&intr, buffer, sizeof(struct gpio_intr));
                ret = intr.level < sizeof(writable_level);
            }

            if (ret)
            {
                u8 port = block / 32, slot = block % 32;

                gpio->handlers[block] = intr.handler;
                gpio->args[block] = intr.arg;

                if (intr.enabled)
                    EINT_CTL(gpio->base, port) |= (1 << slot);
                else
                    EINT_CTL(gpio->base, port) &= ~(1 << slot);

                u8 reg = slot / 8, pos = slot % 8;
                u32 data = writable_level[intr.level] << (pos * 4);
                u32 mask = 0xf << (pos * 4);
                EINT_CFG(gpio->base, port, reg) &= ~mask;
                EINT_CFG(gpio->base, port, reg) |= (data & mask);
            }
            break;
    }

    return ret;
}

drv_decl (gpio, sunxi_gpio)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
