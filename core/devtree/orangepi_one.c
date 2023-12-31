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

#include <core/types.h>
#include <core/utils.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/log.h>

#include <core/fs.h>
#include <core/pic.h>
#include <core/gpio.h>
#include <core/timer.h>
#include <core/serial.h>
#include <core/storage.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

drv_incl (serial, sunxi_uart)
dev_decl (serial, sunxi_uart, tty0)
dev_decl (serial, sunxi_uart, tty1)
dev_decl (serial, sunxi_uart, tty2)
dev_decl (serial, sunxi_uart, tty3)
dev_decl (serial, sunxi_uart, tty4)

drv_incl (gpio, sunxi_gpio)
dev_decl (gpio, sunxi_gpio, gpio0)
dev_decl (gpio, sunxi_gpio, gpio1)

drv_incl (pic, arm_gic)
dev_decl (pic, arm_gic, pic)
drv_incl (timer, sunxi_timer)
dev_decl (timer, sunxi_timer, timer0)
dev_decl (timer, sunxi_timer, timer1)

drv_incl (storage, sunxi_mmc)
dev_decl (storage, sunxi_mmc, mmcblk0)
drv_incl (storage, mbr)
dev_decl (storage, mbr, mmcblk0p1)

drv_incl (storage, memory)
dev_decl (storage, memory, mem)

drv_incl (fs, fat32)
dev_decl (fs, fat32, root)

extern void
_devtree_init(void)
{
    APB0_GATE = 1;

    dev_init (tty0, 0x01c28000);
    dev_config (tty0, .serial.baud   = 115200,
                      .serial.bits   = DRIVER_SERIAL_CHAR_8B,
                      .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                      .serial.stop   = DRIVER_SERIAL_STOP_1B);
    log_set_dev((dev_stream *)&dev(tty0));

    dev_init (tty1, 0x01c28400);
    dev_init (tty2, 0x01c28800);
    dev_init (tty3, 0x01c28c00);
    dev_init (tty4, 0x01f02800);

    dev_init (gpio0, 0x01c20800, 6, 2);
    dev_init (gpio1, 0x01f02c00, 1, 0);

    dev_init (pic,    0x01c82000, 0x01c81000);
    dev_init (timer0, 0x01c20c00, 0, &dev(pic), 50);
    dev_init (timer1, 0x01c20c00, 1, &dev(pic), 51);

    dev_init (mmcblk0,   0x01c0f000);
    dev_init (mmcblk0p1, &dev(mmcblk0), 1);

    dev_init (mem);

    dev_init (root, &dev(mmcblk0p1));

    pin_cfg(&dev(gpio1), 10, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    pin_set(&dev(gpio1), 10, true);
}

extern void
_devtree_clean(void)
{
    dev_clean (tty1)
    dev_clean (tty2)
    dev_clean (tty3)
    dev_clean (tty4)

    pin_set(&dev(gpio1), 10, false);
    pin_cfg(&dev(gpio1), 10, DRIVER_GPIO_OFF, DRIVER_GPIO_PULLOFF);

    pin_cfg(&dev(gpio0), 15, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    pin_set(&dev(gpio0), 15, true);

    dev_clean (gpio0)
    dev_clean (gpio1)

    dev_clean (timer0)
    dev_clean (timer1)

    dev_clean (mmcblk0)
    dev_clean (mmcblk0p1)

    dev_clean (mem)

    dev_clean (root)

    dev_clean (tty0)
    log_set_dev(NULL);

    APB0_GATE = 0;
}
