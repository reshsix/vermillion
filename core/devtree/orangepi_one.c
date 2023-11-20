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

INCLUDE_DRIVER(sunxi_uart)
INCLUDE_DRIVER(sunxi_gpio)
INCLUDE_DRIVER(sunxi_timer)
INCLUDE_DRIVER(sunxi_mmc)

INCLUDE_DRIVER(mbr)
INCLUDE_DRIVER(memory)

INCLUDE_DRIVER(fat32)

DECLARE_DEVICE(tty0)
DECLARE_DEVICE(tty1)
DECLARE_DEVICE(tty2)
DECLARE_DEVICE(tty3)
DECLARE_DEVICE(tty4)

DECLARE_DEVICE(gpio0)
DECLARE_DEVICE(gpio1)

DECLARE_DEVICE(timer0)
DECLARE_DEVICE(timer1)

DECLARE_DEVICE(mmcblk0)
DECLARE_DEVICE(mmcblk0p1)

DECLARE_DEVICE(mem)

DECLARE_DEVICE(root)

extern void
_devtree_init(void)
{
    APB0_GATE = 1;

    INIT_DEVICE(tty0, sunxi_uart, 0x01c28000);
    CONFIG_DEVICE(tty0, .serial.baud   = 115200,
                        .serial.bits   = DRIVER_SERIAL_CHAR_8B,
                        .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                        .serial.stop   = DRIVER_SERIAL_STOP_1B);
    logger(&DEVICE(tty0));

    INIT_DEVICE(tty1, sunxi_uart, 0x01c28400);
    INIT_DEVICE(tty2, sunxi_uart, 0x01c28800);
    INIT_DEVICE(tty3, sunxi_uart, 0x01c28c00);
    INIT_DEVICE(tty4, sunxi_uart, 0x01f02800);

    INIT_DEVICE(gpio0, sunxi_gpio, 0x01c20800, 6, 2);
    INIT_DEVICE(gpio1, sunxi_gpio, 0x01f02c00, 1, 0);

    INIT_DEVICE(timer0, sunxi_timer, 0x01c20c00, 0, 50);
    INIT_DEVICE(timer1, sunxi_timer, 0x01c20c00, 1, 51);

    INIT_DEVICE(mmcblk0,   sunxi_mmc, 0x01c0f000);
    INIT_DEVICE(mmcblk0p1, mbr,       &DEVICE(mmcblk0), 1);

    INIT_DEVICE(mem,  memory);

    INIT_DEVICE(root, fat32, &DEVICE(mmcblk0p1));

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
