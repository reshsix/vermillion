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

#include <core/types.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/mem.h>

#define UART_BUF(p) *(volatile u32*)(p + 0x00)
#define UART_DLL(p) *(volatile u32*)(p + 0x00)
#define UART_DLH(p) *(volatile u32*)(p + 0x04)
#define UART_IER(p) *(volatile u32*)(p + 0x04)
#define UART_IIR(p) *(volatile u32*)(p + 0x08)
#define UART_FCR(p) *(volatile u32*)(p + 0x08)
#define UART_LCR(p) *(volatile u32*)(p + 0x0C)
#define UART_MCR(p) *(volatile u32*)(p + 0x10)
#define UART_LSR(p) *(volatile u32*)(p + 0x14)
#define UART_MSR(p) *(volatile u32*)(p + 0x18)
#define UART_SCH(p) *(volatile u32*)(p + 0x1C)
#define UART_USR(p) *(volatile u32*)(p + 0x7C)
#define UART_TFL(p) *(volatile u32*)(p + 0x80)
#define UART_RFL(p) *(volatile u32*)(p + 0x84)
#define UART_HLT(p) *(volatile u32*)(p + 0xA4)

static inline char
uart_read(u32 p)
{
    while (!(UART_LSR(p) & (1 << 0)));
    return UART_BUF(p);
}

static inline void
uart_write(u32 p, char c)
{
    while (!(UART_LSR(p) & (1 << 5)));
    UART_BUF(p) = c;
}

enum uart_char
{
    UART_CHAR_5B,
    UART_CHAR_6B,
    UART_CHAR_7B,
    UART_CHAR_8B
};

enum uart_parity
{
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
};

enum uart_stop
{
    UART_STOP_1B,
    UART_STOP_1HB,
    UART_STOP_2B
};

enum uart_flags
{
    UART_FLAG_NONE,
    UART_FLAG_AFC,
    UART_FLAG_LOOP
};

static inline void
uart_config(u32 p, u16 divider, enum uart_char c,
            enum uart_parity i, enum uart_stop s, enum uart_flags f)
{
    UART_LCR(p) |= (1 << 7);
    UART_DLH(p) = (divider >> 8) & 0xff;
    UART_DLL(p) = (divider >> 0) & 0xff;
    UART_LCR(p) &= ~(1 << 7);

    UART_LCR(p) &= ~0x3;
    UART_LCR(p) |= c & 0x3;

    switch (i)
    {
        case UART_PARITY_NONE:
            UART_LCR(p) &= ~(1 << 3);
            break;
        case UART_PARITY_ODD:
            UART_LCR(p) &= ~0x38;
            UART_LCR(p) |= (1 << 3);
            break;
        case UART_PARITY_EVEN:
            UART_LCR(p) &= ~0x38;
            UART_LCR(p) |= (1 << 3) | (1 << 4);
            break;
        default:
            break;
    }

    switch (s)
    {
        case UART_STOP_1B:
            UART_LCR(p) &= ~(1 << 2);
            break;
        case UART_STOP_1HB:
        case UART_STOP_2B:
            UART_LCR(p) |= (1 << 2);
            break;
    }

    if (f & UART_FLAG_AFC)
        UART_MCR(p) |= (1 << 5);
    else
        UART_MCR(p) &= ~(1 << 5);

    if (f & UART_FLAG_LOOP)
        UART_MCR(p) |= (1 << 4);
    else
        UART_MCR(p) &= ~(1 << 4);
}

struct uart
{
    u32 port;
    union config config;
};

static void
init(void **ctx, u32 port)
{
    struct uart *ret = mem_new(sizeof(struct uart));

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
config_get(void *ctx, union config *cfg)
{
    struct uart *u = ctx;
    mem_copy(cfg, &(u->config), sizeof(union config));
    return true;
}

static bool
config_set(void *ctx, union config *cfg)
{
    bool ret = true;

    if (!(cfg->serial.baud))
        cfg->serial.baud = 1500000;

    u32 divider = 0;
    if (ret)
    {
        divider = 1500000 / cfg->serial.baud;
        ret = (divider && divider <= UINT16_MAX);
    }

    enum uart_char uc = UART_CHAR_5B;
    if (ret)
    {
        switch (cfg->serial.bits)
        {
            case DRIVER_SERIAL_CHAR_5B:
                break;
            case DRIVER_SERIAL_CHAR_6B:
                uc = UART_CHAR_6B;
                break;
            case DRIVER_SERIAL_CHAR_7B:
                uc = UART_CHAR_7B;
                break;
            case DRIVER_SERIAL_CHAR_8B:
                uc = UART_CHAR_8B;
                break;
            default:
                ret = false;
                break;
        }
    }

    enum uart_parity up = UART_PARITY_NONE;
    if (ret)
    {
        switch (cfg->serial.parity)
        {
            case DRIVER_SERIAL_PARITY_NONE:
                break;
            case DRIVER_SERIAL_PARITY_ODD:
                up = UART_PARITY_ODD;
                break;
            case DRIVER_SERIAL_PARITY_EVEN:
                up = UART_PARITY_EVEN;
                break;
            default:
                ret = false;
                break;
        }
    }

    enum uart_stop us = UART_STOP_1B;
    if (ret)
    {
        switch (cfg->serial.stop)
        {
            case DRIVER_SERIAL_STOP_1B:
                break;
            case DRIVER_SERIAL_STOP_1HB:
                us = UART_STOP_1HB;
                break;
            case DRIVER_SERIAL_STOP_2B:
                us = UART_STOP_2B;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
    {
        struct uart *u = ctx;
        mem_copy(&(u->config), cfg, sizeof(union config));
        uart_config(u->port, divider, uc, up, us, UART_FLAG_NONE);
    }

    return ret;
}

static bool
stream_read(void *ctx, u32 idx, void *data)
{
    bool ret = (idx == 0);

    if (ret)
    {
        struct uart *u = ctx;
        *((u8*)data) = uart_read(u->port);
    }

    return ret;
}

static bool
stream_write(void *ctx, u32 idx, void *data)
{
    bool ret = (idx == 0);

    if (ret)
    {
        struct uart *u = ctx;
        uart_write(u->port, *((u8*)data));
    }

    return ret;
}

drv_decl (serial, sunxi_uart)
{
    .init = init, .clean = clean,
    .config.get = config_get,
    .config.set = config_set,
    .stream.read  = stream_read,
    .stream.write = stream_write
};
