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

#ifndef EASY_IO_H
#define EASY_IO_H

#include <_types.h>
#include <vermillion/drivers.h>

#define LOW  false
#define HIGH true

enum
{
    OFF,
    INPUT_PD,
    INPUT_PU,
    OUTPUT,
    FLOATING,
    EXTRA,
};

void io_chip(struct device *chip);
void io_config(u16 pin, u8 mode);
bool io_read(u16 pin);
void io_write(u16 pin, bool value);

#endif
