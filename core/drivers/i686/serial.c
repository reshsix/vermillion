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

#include <i686/env.h>

#include <core/types.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/mem.h>

#include <core/uart.h>

#define IO_DAT(x)  (x + 0)
#define IO_IER(x)  (x + 1)
#define IO_LCR(x)  (x + 3)
#define IO_MCR(x)  (x + 4)
#define IO_LSR(x)  (x + 5)
#define IO_DIVL(x) (x + 0)
#define IO_DIVH(x) (x + 1)

static u8 write_bits[4] = {0, 1, 2, 3};
static u8 write_parity[5] = {0, 1, 3, 5, 7};
static u8 write_stop[5] = {0, 1, 1};

struct serial
{
    u16 port;
    struct uart_cfg cfg;
};

static void
init(void **ctx, u16 port)
{
    struct serial *ret = mem_new(sizeof(struct serial));

    if (ret)
    {
        ret->port = port;

        out8(IO_IER(port), 0x0);
        out8(IO_MCR(port), 0x1F);

        out8(IO_DAT(port), 0x66);
        if (in8(IO_DAT(port)) == 0x66)
            *ctx = ret;
        else
            mem_del(ret);

        out8(IO_MCR(port), 0xF);
    }
}

static void
clean(void *ctx)
{
    struct serial *com = ctx;
    out8(IO_IER(com->port), 0x0);
    out8(IO_MCR(com->port), 0x0);
    mem_del(com);
}

static bool
stat(void *ctx, u32 idx, u32 *width)
{
    bool ret = true;

    (void)ctx;
    switch (idx)
    {
        case 0:
            *width = sizeof(struct uart_cfg);
            break;
        case 1:
            *width = sizeof(u8);
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

    struct serial *com = ctx;
    switch (idx)
    {
        case 0:
            ret = true;
            mem_copy(data, &com->cfg, sizeof(u8));
            break;

        case 1:
            ret = true;
            while (!(in8(IO_LSR(com->port)) & 0x1));

            u8 byte = in8(IO_DAT(com->port));
            mem_copy(data, &byte, sizeof(u8));
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    struct serial *com = ctx;
    switch (idx)
    {
        case 0:;
            struct uart_cfg cfg = {0};
            mem_copy(&cfg, data, sizeof(struct uart_cfg));

            if (cfg.baud == 0)
                cfg.baud = 115200;

            ret = (cfg.baud   <= 115200               &&
                   cfg.baud   >= 2                    &&
                   cfg.bits   <  sizeof(write_bits)   &&
                   cfg.parity <  sizeof(write_parity) &&
                   cfg.stop   <  sizeof(write_stop)   &&
                   !(cfg.stop == UART_1HS && cfg.bits != UART_5B) &&
                   !(cfg.stop == UART_2S  && cfg.bits == UART_5B));

            if (ret)
            {
                mem_copy(&(com->cfg), &cfg, sizeof(struct uart_cfg));

                u8 c = write_bits[cfg.bits];
                u8 p = write_parity[cfg.parity];
                u8 s = write_stop[cfg.stop];

                u16 divisor = 115200 / cfg.baud;
                u8 LCR = in8(IO_LCR(com->port));
                out8(IO_LCR(com->port), LCR | 0x80);

                out8(IO_DIVL(com->port), divisor & 0xFF);
                out8(IO_DIVH(com->port), divisor >> 8);

                LCR |= c | p << 5 | s << 2;

                out8(IO_LCR(com->port), LCR & ~0x80);
            }
            break;

        case 1:
            ret = true;
            while (!(in8(IO_LSR(com->port)) & 0x20));

            u8 byte = 0;
            mem_copy(&byte, data, sizeof(u8));
            out8(IO_DAT(com->port), byte);
            break;
    }

    return ret;
}

drv_decl (uart, i686_com)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
