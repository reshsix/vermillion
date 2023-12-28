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

#include <vermillion/utils.h>
#include <vermillion/drivers.h>
#include <vermillion/i686/env.h>

#define IO_DAT(x)  (x + 0)
#define IO_IER(x)  (x + 1)
#define IO_LCR(x)  (x + 3)
#define IO_MCR(x)  (x + 4)
#define IO_LSR(x)  (x + 5)
#define IO_DIVL(x) (x + 0)
#define IO_DIVH(x) (x + 1)

struct serial
{
    u16 port;
    union config config;
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
    if (ctx)
    {
        struct serial *com = ctx;
        out8(IO_IER(com->port), 0x0);
        out8(IO_MCR(com->port), 0x0);
    }
    mem_del(ctx);
}

static bool
config_get(void *ctx, union config *cfg)
{
    struct serial *com = ctx;
    mem_copy(cfg, &(com->config), sizeof(union config));
    return true;
}

static bool
config_set(void *ctx, union config *cfg)
{
    bool ret = true;

    if (ret)
    {
        if (!(cfg->serial.baud))
            cfg->serial.baud = 115200;
        else if (cfg->serial.baud > 115200 ||
                 115200 % cfg->serial.baud != 0)
            ret = false;
    }

    u8 c = 0;
    if (ret)
    {
        switch (cfg->serial.bits)
        {
            case DRIVER_SERIAL_CHAR_5B:
                break;
            case DRIVER_SERIAL_CHAR_6B:
                c = 1;
                break;
            case DRIVER_SERIAL_CHAR_7B:
                c = 2;
                break;
            case DRIVER_SERIAL_CHAR_8B:
                c = 3;
                break;
            default:
                ret = false;
                break;
        }
    }

    u8 p = 0;
    if (ret)
    {
        switch (cfg->serial.parity)
        {
            case DRIVER_SERIAL_PARITY_NONE:
                break;
            case DRIVER_SERIAL_PARITY_ODD:
                p = 1;
                break;
            case DRIVER_SERIAL_PARITY_EVEN:
                p = 3;
                break;
            case DRIVER_SERIAL_PARITY_MARK:
                p = 5;
                break;
            case DRIVER_SERIAL_PARITY_SPACE:
                p = 7;
                break;
            default:
                ret = false;
                break;
        }
    }

    u8 s = 0;
    if (ret)
    {
        switch (cfg->serial.stop)
        {
            case DRIVER_SERIAL_STOP_1B:
                break;
            case DRIVER_SERIAL_STOP_1HB:
                if (cfg->serial.bits == DRIVER_SERIAL_CHAR_5B)
                    s = 1;
                else
                    ret = false;
                break;
            case DRIVER_SERIAL_STOP_2B:
                if (cfg->serial.bits != DRIVER_SERIAL_CHAR_5B)
                    s = 1;
                else
                    ret = false;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
    {
        struct serial *com = ctx;
        mem_copy(&(com->config), cfg, sizeof(union config));

        u16 divisor = 115200 / cfg->serial.baud;
        u8 LCR = in8(IO_LCR(com->port));
        out8(IO_LCR(com->port), LCR | 0x80);

        out8(IO_DIVL(com->port), divisor & 0xFF);
        out8(IO_DIVH(com->port), divisor >> 8);

        LCR |= c | p << 5 | s << 2;

        out8(IO_LCR(com->port), LCR & ~0x80);
    }

    return ret;
}

static bool
stream_read(void *ctx, u32 idx, u8 *data)
{
    bool ret = (idx == 0);

    if (ret)
    {
        struct serial *com = ctx;
        while (!(in8(IO_LSR(com->port)) & 0x1));
        data[0] = in8(IO_DAT(com->port));
    }

    return ret;
}

static bool
stream_write(void *ctx, u32 idx, u8 *data)
{
    bool ret = (idx == 0);

    if (ret)
    {
        struct serial *com = ctx;
        while (!(in8(IO_LSR(com->port)) & 0x20));
        out8(IO_DAT(com->port), data[0]);
    }

    return ret;
}

DECLARE_DRIVER(i686_com)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_STREAM,
    .type = DRIVER_TYPE_SERIAL,
    .config.get = config_get,
    .config.set = config_set,
    .stream.read  = stream_read,
    .stream.write = stream_write
};
