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
#include <general/mem.h>

#include <hal/classes/pic.h>
#include <hal/classes/uart.h>

#define IO_BUF(p) *(volatile u32*)(p + 0x00)
#define IO_DLL(p) *(volatile u32*)(p + 0x00)
#define IO_DLH(p) *(volatile u32*)(p + 0x04)
#define IO_IER(p) *(volatile u32*)(p + 0x04)
#define IO_IIR(p) *(volatile u32*)(p + 0x08)
#define IO_FCR(p) *(volatile u32*)(p + 0x08)
#define IO_LCR(p) *(volatile u32*)(p + 0x0C)
#define IO_MCR(p) *(volatile u32*)(p + 0x10)
#define IO_LSR(p) *(volatile u32*)(p + 0x14)
#define IO_MSR(p) *(volatile u32*)(p + 0x18)
#define IO_SCH(p) *(volatile u32*)(p + 0x1C)
#define IO_USR(p) *(volatile u32*)(p + 0x7C)
#define IO_TFL(p) *(volatile u32*)(p + 0x80)
#define IO_RFL(p) *(volatile u32*)(p + 0x84)
#define IO_HLT(p) *(volatile u32*)(p + 0xA4)

/* Driver definition */

struct serial
{
    u32 port;
    struct uart_cfg cfg;

    dev_pic *pic;
    u8 irq;

    u8 buffer[0x400];
    size_t head, tail;
};

struct serial serials[5] = {0};
static const u32 ports[5] = {0x01c28000, 0x01c28400,
                             0x01c28800, 0x01c28c00, 0x01f02800};
static const u8 irqs[5] = {32, 33, 34, 35, 70};

static void
uart_handler(void *arg)
{
    struct serial *u = arg;

    while (IO_LSR(u->port) & (1 << 0))
    {
        size_t next = (u->head + 1) & 0x3FF;
        if (next != u->tail)
        {
            u->buffer[u->head] = IO_BUF(u->port);
            u->head = next;
        }
    }
}

static bool
ioctl(void *ctx, u8 idx, void *data)
{
    bool ret = false;

    struct serial *u = ctx;
    switch (idx)
    {
        case UART_CONFIG_GET:
            ret = true;
            mem_copy(data, &(u->cfg), sizeof(struct uart_cfg));
            break;
        case UART_CONFIG_SET:;
            struct uart_cfg cfg = {0};
            mem_copy(&cfg, data, sizeof(struct uart_cfg));

            if (cfg.baud == 0)
                cfg.baud = 115200;

            ret = (  cfg.baud <= 1500000  && cfg.baud >= 23       &&
                   !(cfg.stop == UART_1HS && cfg.bits != UART_5B) &&
                   !(cfg.stop == UART_2S  && cfg.bits == UART_5B));
            if (ret)
            {
                mem_copy(&(u->cfg), &cfg, sizeof(struct uart_cfg));
                while (IO_USR(u->port) & 1);

                u16 divider = 1500000 / cfg.baud;
                IO_LCR(u->port) |= (1 << 7);
                IO_DLH(u->port) = (divider >> 8) & 0xff;
                IO_DLL(u->port) = (divider >> 0) & 0xff;
                IO_LCR(u->port) &= ~(1 << 7);

                IO_LCR(u->port) &= ~0x3;
                IO_LCR(u->port) |= cfg.bits & 0x3;

                switch (cfg.parity)
                {
                    case UART_NOPARITY:
                        IO_LCR(u->port) &= ~(1 << 3);
                        break;
                    case UART_ODD:
                        IO_LCR(u->port) &= ~0x38;
                        IO_LCR(u->port) |= (1 << 3);
                        break;
                    case UART_EVEN:
                        IO_LCR(u->port) &= ~0x38;
                        IO_LCR(u->port) |= (1 << 3) | (1 << 4);
                        break;
                    default:
                        break;
                }

                switch (cfg.stop)
                {
                    case UART_1S:
                        IO_LCR(u->port) &= ~(1 << 2);
                        break;
                    case UART_1HS:
                    case UART_2S:
                        IO_LCR(u->port) |= (1 << 2);
                        break;
                }
            }
            break;
    }

    return ret;
}

static bool
stat(void *ctx, size_t *width)
{
    *width = sizeof(u8);
    return true;
}

static bool
read(void *ctx, void *data)
{
    bool ret = false;

    struct serial *u = ctx;
    ret = (u->head != u->tail || IO_LSR(u->port) & (1 << 0));
    if (ret)
    {
        u8 byte = 0;
        if (u->head != u->tail)
        {
            byte = u->buffer[u->tail];
            u->tail = (u->tail + 1) & 0x3FF;
        }
        else
            byte = IO_BUF(u->port);
        mem_copy(data, &byte, sizeof(u8));
    }

    return ret;
}

static bool
write(void *ctx, void *data)
{
    bool ret = false;

    struct serial *u = ctx;
    ret = (IO_LSR(u->port) & (1 << 5));
    if (ret)
    {
        u8 byte = 0;
        mem_copy(&byte, data, sizeof(u8));
        IO_BUF(u->port) = byte;
    }

    return ret;
}

static const drv_uart sunxi_uart =
{
    .ioctl = ioctl, .stat = stat, .read = read, .write = write
};

/* Device creation */

extern dev_uart
sunxi_uart_init(u8 id, dev_pic *pic)
{
    struct serial *ret = NULL;

    if (id < (sizeof(serials) / sizeof(struct serial)))
    {
        ret = &(serials[id]);
        ret->port = ports[id];

        ret->pic = pic;
        ret->irq = irqs[id];
        pic_config(pic, ret->irq, true, uart_handler, ret, PIC_LEVEL_H);

        /* FIFOs with RX 1/2 full interrupt */
        IO_FCR(ret->port) = (1 << 7) | (1 << 0);
        IO_IER(ret->port) |= (1 << 0);
    }

    return (dev_uart){.driver = &sunxi_uart, .context = ret};
}

extern void
sunxi_uart_clean(dev_uart *u)
{
    if (u)
    {
        struct serial *s = u->context;
        pic_config(s->pic, s->irq, false, NULL, NULL, PIC_LEVEL_H);
        u->context = NULL;
    }
}
