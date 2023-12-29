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

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

INCLUDE_DRIVER(serial, sunxi_uart)
DECLARE_DEVICE(serial, sunxi_uart, tty0)
DECLARE_DEVICE(serial, sunxi_uart, tty1)
DECLARE_DEVICE(serial, sunxi_uart, tty2)
DECLARE_DEVICE(serial, sunxi_uart, tty3)
DECLARE_DEVICE(serial, sunxi_uart, tty4)

INCLUDE_DRIVER(gpio, sunxi_gpio)
DECLARE_DEVICE(gpio, sunxi_gpio, gpio0)
DECLARE_DEVICE(gpio, sunxi_gpio, gpio1)

INCLUDE_DRIVER(pic, arm_gic)
DECLARE_DEVICE(pic, arm_gic, pic)
INCLUDE_DRIVER(timer, sunxi_timer)
DECLARE_DEVICE(timer, sunxi_timer, timer0)
DECLARE_DEVICE(timer, sunxi_timer, timer1)

INCLUDE_DRIVER(storage, sunxi_mmc)
DECLARE_DEVICE(storage, sunxi_mmc, mmcblk0)
INCLUDE_DRIVER(storage, mbr)
DECLARE_DEVICE(storage, mbr, mmcblk0p1)

INCLUDE_DRIVER(storage, memory)
DECLARE_DEVICE(storage, memory, mem)

INCLUDE_DRIVER(fs, fat32)
DECLARE_DEVICE(fs, fat32, root)

extern void
_devtree_init(void)
{
    APB0_GATE = 1;

    INIT_DEVICE(tty0, 0x01c28000);
    CONFIG_DEVICE(tty0, .serial.baud   = 115200,
                        .serial.bits   = DRIVER_SERIAL_CHAR_8B,
                        .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                        .serial.stop   = DRIVER_SERIAL_STOP_1B);
    logger(&DEVICE(tty0));

    INIT_DEVICE(tty1, 0x01c28400);
    INIT_DEVICE(tty2, 0x01c28800);
    INIT_DEVICE(tty3, 0x01c28c00);
    INIT_DEVICE(tty4, 0x01f02800);

    INIT_DEVICE(gpio0, 0x01c20800, 6, 2);
    INIT_DEVICE(gpio1, 0x01f02c00, 1, 0);

    INIT_DEVICE(pic,    0x01c82000, 0x01c81000);
    INIT_DEVICE(timer0, 0x01c20c00, 0, &DEVICE(pic), 50);
    INIT_DEVICE(timer1, 0x01c20c00, 1, &DEVICE(pic), 51);

    INIT_DEVICE(mmcblk0,   0x01c0f000);
    INIT_DEVICE(mmcblk0p1, &DEVICE(mmcblk0), 1);

    INIT_DEVICE(mem);

    INIT_DEVICE(root, &DEVICE(mmcblk0p1));

    pin_cfg(&DEVICE(gpio1), 10, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    pin_set(&DEVICE(gpio1), 10, true);
}

extern void
_devtree_clean(void)
{
    CLEAN_DEVICE(tty1)
    CLEAN_DEVICE(tty2)
    CLEAN_DEVICE(tty3)
    CLEAN_DEVICE(tty4)

    pin_set(&DEVICE(gpio1), 10, false);
    pin_cfg(&DEVICE(gpio1), 10, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);

    pin_cfg(&DEVICE(gpio0), 15, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    pin_set(&DEVICE(gpio0), 15, true);

    CLEAN_DEVICE(gpio0)
    CLEAN_DEVICE(gpio1)

    CLEAN_DEVICE(timer0)
    CLEAN_DEVICE(timer1)

    CLEAN_DEVICE(mmcblk0)
    CLEAN_DEVICE(mmcblk0p1)

    CLEAN_DEVICE(mem)

    CLEAN_DEVICE(root)

    CLEAN_DEVICE(tty0)
    logger(NULL);

    APB0_GATE = 0;
}
