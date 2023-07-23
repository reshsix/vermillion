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

#include <stdio.h>
#include <vermillion/drivers.h>

extern void
_devices_init(void)
{
    DEVICE_NEW("tty0", "i686-com", 0x3F8);
    DEVICE_CONFIG("tty0", .serial.baud   = 115200,
                          .serial.bits   = DRIVER_SERIAL_CHAR_8B,
                          .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                          .serial.stop   = DRIVER_SERIAL_STOP_1B);
    DEVICE_LOGGER("tty0");

    DEVICE_NEW("tty1", "i686-com", 0x2F8);
    DEVICE_NEW("tty2", "i686-com", 0x3E8);
    DEVICE_NEW("tty3", "i686-com", 0x2E8);

    _stdio_init(NULL, "tty0", "tty0", "tty0");
}

extern void
_devices_clean(void)
{
    _stdio_clean();
    DEVTREE_CLEANUP();
}
