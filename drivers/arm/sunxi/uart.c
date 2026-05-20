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

#define VERMILLION_INTERNALS
#include <vermillion/hal/uart.h>

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

struct uart
{
    u32 port;
    u32 baud;
    u32 fields;

    dev_pic *pic;
    u8 irq;

    u8 buffer[0x400];
    size_t head, tail;
};

struct uart serials[5] = {0};
static const u32 ports[5] = {0x01c28000, 0x01c28400,
                             0x01c28800, 0x01c28c00, 0x01f02800};
static const u8 irqs[5] = {32, 33, 34, 35, 70};

static void
uart_handler(void *arg)
{
    struct uart *u = arg;

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
info(void *ctx, u32 *baud, u32 *fields)
{
    struct uart *u = ctx;
    *baud   = u->baud;
    *fields = u->fields;
    return true;
}

static bool
config(void *ctx, u32 baud, u32 fields)
{
    bool ret = (baud <= 1500000 && baud >= 23);

    if (ret)
    {
        struct uart *u = ctx;
        while (IO_USR(u->port) & 1);

        u16 divider = 1500000 / baud;
        u8     bits = (fields >> 0) & 0x7;
        u8   parity = (fields >> 3) & 0x7;
        u8     stop = (fields >> 6) & 0x3;
        ret = ((bits   <= VRM_UART_5B)   &&
               (parity <= VRM_UART_EVEN) &&
               (stop   <= VRM_UART_2S)   &&
               ((stop != VRM_UART_1HS) || (bits == VRM_UART_5B)));

        if (ret)
        {
            u8   bitses[4] = {3, 2, 1, 0};
            u8 parities[3] = {0, 1, 3};
            u8    stops[3] = {0, 1, 1};

            bits   = bitses[bits];
            parity = parities[parity];
            stop   = stops[stop];

            IO_LCR(u->port) |= (1 << 7);
            IO_DLH(u->port) = (divider >> 8) & 0xff;
            IO_DLL(u->port) = (divider >> 0) & 0xff;
            IO_LCR(u->port) &= ~(1 << 7);
            IO_LCR(u->port) = bits | (stop << 2) | (parity << 3);

            u->baud   = 1500000 / divider;
            u->fields = fields;
        }
    }

    return ret;
}

static bool
read(void *ctx, u8 *data)
{
    bool ret = false;

    struct uart *u = ctx;
    ret = (u->head != u->tail || IO_LSR(u->port) & (1 << 0));
    if (ret)
    {
        if (u->head != u->tail)
        {
            *data = u->buffer[u->tail];
            u->tail = (u->tail + 1) & 0x3FF;
        }
        else
            *data = IO_BUF(u->port);
    }

    return ret;
}

static bool
write(void *ctx, u8 data)
{
    bool ret = false;

    struct uart *u = ctx;
    ret = (IO_LSR(u->port) & (1 << 5));
    if (ret)
        IO_BUF(u->port) = data;

    return ret;
}

static const drv_uart sunxi_uart =
{
    .info = info, .config = config,
    .read = read, .write = write
};

/* Device creation */

extern dev_uart
sunxi_uart_init(u8 id, dev_pic *pic)
{
    struct uart *ret = NULL;

    if (id < (sizeof(serials) / sizeof(struct uart)))
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
        struct uart *u2 = u->context;
        pic_config(u2->pic, u2->irq, false, NULL, NULL, PIC_LEVEL_H);
        u->context = NULL;
    }
}
