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

#ifndef _I686_ENV_H
#define _I686_ENV_H

#include <vermillion/types.h>

static inline u8
in8(u16 port)
{
    u8 ret  = 0;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline u16
in16(u16 port)
{
    u16 ret  = 0;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline u32
in32(u16 port)
{
    u32 ret  = 0;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void
out8(u16 port, u8 data)
{
    asm volatile ("outb %0, %1" :: "a"(data), "Nd"(port));
}

static inline void
out16(u16 port, u16 data)
{
    asm volatile ("outw %0, %1" :: "a"(data), "Nd"(port));
}

static inline void
out32(u16 port, u32 data)
{
    asm volatile ("outl %0, %1" :: "a"(data), "Nd"(port));
}

static inline void
io_wait(void)
{
    out8(0x80, 0);
}

#endif
