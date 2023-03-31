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

#include <_types.h>
#include <_i686-env.h>
#include <vermillion/drivers.h>

#define IO_DAT(x)  (x + 0)
#define IO_IER(x)  (x + 1)
#define IO_IIR(x)  (x + 2)
#define IO_LCR(x)  (x + 3)
#define IO_MCR(x)  (x + 4)
#define IO_LSR(x)  (x + 5)
#define IO_DIVL(x) (x + 0)
#define IO_DIVH(x) (x + 1)

static u16 ports[] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};

static bool
serial_init(u8 port)
{
    bool ret = true;

    out8(IO_IER(ports[port]), 0x0);
    out8(IO_MCR(ports[port]), 0x1F);
    out8(IO_DAT(ports[port]), 0x66);
    ret = in8(IO_DAT(ports[port])) == 0x66;
    out8(IO_MCR(ports[port]), 0xF);

    return ret;
}

static void
serial_clean(u8 port)
{
    out8(IO_IER(ports[port]), 0x0);
    out8(IO_MCR(ports[port]), 0x0);
}

static bool
serial_config(u8 port, u32 baud, u8 ch, u8 parity, u8 stop)
{
    bool ret = true;

    if (!baud || baud > 112500 || 115200 % baud != 0)
        ret = false;

    u8 c = 0;
    if (ret)
    {
        switch (ch)
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
        switch (parity)
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
        switch (stop)
        {
            case DRIVER_SERIAL_STOP_1B:
                break;
            case DRIVER_SERIAL_STOP_1HB:
                if (ch == DRIVER_SERIAL_CHAR_5B)
                    s = 1;
                else
                    ret = false;
                break;
            case DRIVER_SERIAL_STOP_2B:
                if (ch != DRIVER_SERIAL_CHAR_5B)
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
        u16 divisor = 115200 / baud;
        u8 LCR = in8(IO_LCR(ports[port]));
        out8(IO_LCR(ports[port]), LCR | 0x80);

        out8(IO_DIVL(ports[port]), divisor & 0xFF);
        out8(IO_DIVH(ports[port]), divisor >> 8);

        LCR |= c | p << 5 | s << 2;

        out8(IO_LCR(ports[port]), LCR & ~0x80);
    }

    return ret;
}

static u8
serial_read(u8 port)
{
    while (!(in8(IO_LSR(ports[port])) & 0x1));
    return in8(IO_DAT(ports[port]));
}

static void
serial_write(u8 port, u16 data)
{
    while (!(in8(IO_LSR(ports[port])) & 0x20));
    out8(IO_DAT(ports[port]), data);
}

static bool com1_init(void){ return serial_init(0); }
static bool com2_init(void){ return serial_init(1); }
static bool com3_init(void){ return serial_init(2); }
static bool com4_init(void){ return serial_init(3); }
static void com1_clean(void){ serial_clean(0); }
static void com2_clean(void){ serial_clean(1); }
static void com3_clean(void){ serial_clean(2); }
static void com4_clean(void){ serial_clean(3); }
static bool com1_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(0, baud, ch, parity, stop); }
static bool com2_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(1, baud, ch, parity, stop); }
static bool com3_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(2, baud, ch, parity, stop); }
static bool com4_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(3, baud, ch, parity, stop); }
static u8 com1_read(void){ return serial_read(0); }
static u8 com2_read(void){ return serial_read(1); }
static u8 com3_read(void){ return serial_read(2); }
static u8 com4_read(void){ return serial_read(3); }
static void com1_write(u16 data){ return serial_write(0, data); }
static void com2_write(u16 data){ return serial_write(1, data); }
static void com3_write(u16 data){ return serial_write(2, data); }
static void com4_write(u16 data){ return serial_write(3, data); }

static const struct driver x86_com1 =
{
    .name = "x86 COM1 Port",
    .init = com1_init, .clean = com1_clean,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = com1_config,
    .routines.serial.read   = com1_read,
    .routines.serial.write  = com1_write
};
driver_register(x86_com1);

static const struct driver x86_com2 =
{
    .name = "x86 COM2 Port",
    .init = com2_init, .clean = com2_clean,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = com2_config,
    .routines.serial.read   = com2_read,
    .routines.serial.write  = com2_write
};
driver_register(x86_com2);

static const struct driver x86_com3 =
{
    .name = "x86 COM3 Port",
    .init = com3_init, .clean = com3_clean,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = com3_config,
    .routines.serial.read   = com3_read,
    .routines.serial.write  = com3_write
};
driver_register(x86_com3);

static const struct driver x86_com4 =
{
    .name = "x86 COM4 Port",
    .init = com4_init, .clean = com4_clean,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = com4_config,
    .routines.serial.read   = com4_read,
    .routines.serial.write  = com4_write
};
driver_register(x86_com4);

