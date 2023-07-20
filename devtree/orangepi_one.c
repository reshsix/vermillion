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
#include <_utils.h>
#include <vermillion/drivers.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

extern void
_devices_init(void)
{
    APB0_GATE = 1;

    DEVICE_NEW("tty0", "sunxi-uart", 0x01c28000);
    DEVICE_CONFIG("tty0", .serial.baud =   115200,
                          .serial.bits =   DRIVER_SERIAL_CHAR_8B,
                          .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                          .serial.stop   = DRIVER_SERIAL_STOP_1B);
    DEVICE_LOGGER("tty0");

    DEVICE_NEW("tty1", "sunxi-uart", 0x01c28400);
    DEVICE_NEW("tty2", "sunxi-uart", 0x01c28800);
    DEVICE_NEW("tty3", "sunxi-uart", 0x01c28c00);
    DEVICE_NEW("tty4", "sunxi-uart", 0x01f02800);

    DEVICE_NEW("gpio0", "sunxi-gpio", 0x01c20800, 6, 2);
    DEVICE_NEW("gpio1", "sunxi-gpio", 0x01f02c00, 1, 0);

    DEVICE_NEW("timer0", "sunxi-timer", 0x01c20c00, 0, 50);
    DEVICE_NEW("timer1", "sunxi-timer", 0x01c20c00, 1, 51);

    DEVICE_NEW("mmcblk0",   "sunxi-mmc", 0x01c0f000);
    DEVICE_NEW("mmcblk0p1", "mbr",       DEVICE("mmcblk0"), 1);

    DEVICE_NEW("null", "null");
    DEVICE_NEW("zero", "zero");
    DEVICE_NEW("mem",  "memory");

    DEVICE_NEW("root", "fat32", DEVICE("mmcblk0p1"));

    _stdio_init(DEVICE("root"), "tty0", "tty0", "tty0");

    pin_cfg(DEVICE("gpio1"), 10, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    pin_set(DEVICE("gpio1"), 10, true);
}

extern void
_devices_clean(void)
{
    _stdio_clean();

    pin_set(DEVICE("gpio1"), 10, false);
    pin_cfg(DEVICE("gpio1"), 10, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);

    pin_cfg(DEVICE("gpio0"), 15, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    pin_set(DEVICE("gpio0"), 15, true);

    DEVTREE_CLEANUP();

    APB0_GATE = 0;
}
