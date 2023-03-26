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

#ifndef INTERFACE_SERIAL_H
#define INTERFACE_SERIAL_H

#include <_types.h>
#include <drivers/sunxi-uart.h>

bool _serial_init(void);
void _serial_clean(void);

enum serial_char
{
    SERIAL_CHAR_5B,
    SERIAL_CHAR_6B,
    SERIAL_CHAR_7B,
    SERIAL_CHAR_8B,
    SERIAL_CHAR_9B
};

enum serial_parity
{
    SERIAL_PARITY_NONE,
    SERIAL_PARITY_ODD,
    SERIAL_PARITY_EVEN
};

enum serial_stop
{
    SERIAL_STOP_1B,
    SERIAL_STOP_1HB,
    SERIAL_STOP_2B
};

bool serial_config(u8 port, u32 baud, enum serial_char c,
                   enum serial_parity p, enum serial_stop s);
u8 serial_read(u8 port);
void serial_write(u8 port, u16 data);

#endif
