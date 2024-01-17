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
#include <hal/generic/stream.h>
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

struct serial
{
    u32 port;
    struct uart_cfg cfg;
};

static void
init(void **ctx, u32 port)
{
    struct serial *ret = mem_new(sizeof(struct serial));

    if (ret)
    {
        ret->port = port;
        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    mem_del(ctx);
}

static bool
stat(void *ctx, u32 idx, u32 *width)
{
    bool ret = true;

    (void)ctx;
    switch (idx)
    {
        case STREAM_COMMON:
            *width = sizeof(u8);
            break;
        case UART_CONFIG:
            *width = sizeof(struct uart_cfg);
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    struct serial *u = ctx;
    switch (idx)
    {
        case STREAM_COMMON:
            ret = true;
            while (!(IO_LSR(u->port) & (1 << 0)));

            u8 byte = IO_BUF(u->port);
            mem_copy(data, &byte, sizeof(u8));
            break;

        case UART_CONFIG:
            ret = true;
            mem_copy(data, &(u->cfg), sizeof(struct uart_cfg));
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    struct serial *u = ctx;
    switch (idx)
    {
        case STREAM_COMMON:
            ret = true;
            while (!(IO_LSR(u->port) & (1 << 5)));

            u8 byte = IO_BUF(u->port);
            mem_copy(&byte, data, sizeof(u8));
            IO_BUF(u->port) = byte;
            break;

        case UART_CONFIG:;
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

drv_decl (uart, sunxi_uart)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
