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

#ifndef COMMON_PORTS_H
#define COMMON_PORTS_H

#include "common/types.h"

enum
{
    PORT_CRTL0 = 0x01C20800,
    PORT_CRTL1 = 0x01F02C00
};

#define PN_CFG(c, n, i) *(volatile u32*)(c + (n * 0x24) + (0x4 * i))
#define PN_DAT(c, n)    *(volatile u32*)(c + (n * 0x24) + 0x10)
#define PN_DRV(c, n, i) *(volatile u32*)(c + (n * 0x24) + 0x14 + (0x4 * i))
#define PN_PUL(c, n, i) *(volatile u32*)(c + (n * 0x24) + 0x1C + (0x4 * i))

#define EINT_CFG(c, n, i) *(volatile u32*)(c + 0x200 + (n * 0x20) + (0x4 * i))
#define EINT_CTL(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x10)
#define EINT_STA(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x14)
#define EINT_DEB(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x18)

enum
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

static inline void
pin_config(u32 base, u8 pin, u8 cfg)
{
    if (cfg > PIN_CFG_OFF)
        cfg = PIN_CFG_OFF;

    u8 port = pin / 32;
    u8 slot = pin % 32;
    u8 reg = slot / 8;
    u8 pos = slot % 8;

    u32 data = cfg << (pos * 4);
    u32 mask = 0xf << (pos * 4);

    PN_CFG(base, port, reg) &= ~mask;
    PN_CFG(base, port, reg) |= (data & mask);
}

static inline bool
pin_read(u32 base, u8 pin)
{
    u8 port = pin / 32;
    u8 slot = pin % 32;
    return (PN_DAT(base, port) >> slot) & 0x1;
}

static inline void
pin_write(u32 base, u8 pin, bool bit)
{
    u8 port = pin / 32;
    u8 slot = pin % 32;

    if (bit)
        PN_DAT(base, port) |= 0x1 << slot;
    else
        PN_DAT(base, port) &= ~(0x1 << slot);
}

enum
{
    PIN_LEVEL0 = 0,
    PIN_LEVEL1,
    PIN_LEVEL2,
    PIN_LEVEL3
};

static inline void
pin_level(u32 base, u8 pin, u8 level)
{
    if (level > PIN_LEVEL3)
        level = PIN_LEVEL3;

    u8 port = pin / 32;
    u8 slot = pin % 32;
    u8 reg = slot / 16;
    u8 pos = slot % 16;

    u32 data = level << (pos * 2);
    u32 mask = 0x3 << (pos * 2);

    PN_DRV(base, port, reg) &= ~mask;
    PN_DRV(base, port, reg) |= (data & mask);
}

enum
{
    PIN_PULLOFF = 0,
    PIN_PULLUP,
    PIN_PULLDOWN
};

static inline void
pin_pull(u32 base, u8 pin, u8 pull)
{
    if (pull > PIN_PULLDOWN)
        pull = PIN_PULLOFF;

    u8 port = pin / 32;
    u8 slot = pin % 32;
    u8 reg = slot / 16;
    u8 pos = slot % 16;

    u32 data = pull << (pos * 2);
    u32 mask = 0x3 << (pos * 2);

    PN_PUL(base, port, reg) &= ~mask;
    PN_PUL(base, port, reg) |= (data & mask);
}

enum
{
    EINT_EDGE_H,
    EINT_EDGE_L,
    EINT_LEVEL_H,
    EINT_LEVEL_L,
    EINT_DOUBLE
};

static inline void
eint_config(u32 base, u8 eint, u8 cfg)
{
    if (cfg > EINT_DOUBLE)
        cfg = EINT_EDGE_H;

    u8 port = eint / 32;
    u8 slot = eint % 32;
    u8 reg = slot / 8;
    u8 pos = slot % 8;

    u32 data = cfg << (pos * 4);
    u32 mask = 0xf << (pos * 4);

    EINT_CFG(base, port, reg) &= ~mask;
    EINT_CFG(base, port, reg) |= (data & mask);
}

static inline void
eint_control(u32 base, u8 eint, bool status)
{
    u8 port = eint / 32;
    u8 slot = eint % 32;

    if (status)
        EINT_CTL(base, port) |= 0x1 << slot;
    else
        EINT_CTL(base, port) &= ~(0x1 << slot);
}

static inline void
eint_ack(u32 base, u8 eint)
{
    u8 port = eint / 32;
    u8 slot = eint % 32;
    EINT_STA(base, port) |= 0x1 << slot;
}

static inline void
eint_debounce(u32 base, u8 eint, u8 prescale, bool osc24m)
{
    u8 port = eint / 32;
    EINT_DEB(base, port) = (prescale & 0x7) << 4 | osc24m;
}

#endif
