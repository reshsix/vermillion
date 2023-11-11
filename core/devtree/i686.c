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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>

INCLUDE_DRIVER(i686_com)

DECLARE_DEVICE(tty0)
DECLARE_DEVICE(tty1)
DECLARE_DEVICE(tty2)
DECLARE_DEVICE(tty3)

extern void
_devtree_init(void)
{
    INIT_DEVICE(tty0, i686_com, 0x3F8)
    CONFIG_DEVICE(tty0, .serial.baud =   115200,
                        .serial.bits =   DRIVER_SERIAL_CHAR_8B,
                        .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                        .serial.stop   = DRIVER_SERIAL_STOP_1B);
    logger(&tty0);

    INIT_DEVICE(tty1, i686_com, 0x2F8)
    INIT_DEVICE(tty2, i686_com, 0x3E8)
    INIT_DEVICE(tty3, i686_com, 0x2E8)
}

extern void
_devtree_clean(void)
{
    CLEAN_DEVICE(tty1)
    CLEAN_DEVICE(tty3)
    CLEAN_DEVICE(tty2)

    CLEAN_DEVICE(tty0)
    logger(NULL);
}
