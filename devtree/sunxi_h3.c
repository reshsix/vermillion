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

#include <general/types.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/generic/block.h>
#include <hal/generic/stream.h>
#include <hal/classes/fs.h>
#include <hal/classes/pic.h>
#include <hal/classes/gpio.h>
#include <hal/classes/uart.h>
#include <hal/classes/timer.h>

#include <system/comm.h>
#include <system/wheel.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

#define CCU 0x01C20000
#define CLK_SPI0   *(volatile u32*)(CCU + 0xA0)
#define BUS0_GATE  *(volatile u32*)(CCU + 0x60)
#define BUS0_RESET *(volatile u32*)(CCU + 0x2C0)

#define ORANGEPI_ONE 0
#define NANOPI_NEO 1

drv_incl (uart, sunxi_uart);
dev_decl (uart, sunxi_uart, tty0);
dev_decl (uart, sunxi_uart, tty1);
dev_decl (uart, sunxi_uart, tty2);
dev_decl (uart, sunxi_uart, tty3);
dev_decl (uart, sunxi_uart, tty4);

drv_incl (gpio, sunxi_gpio);
dev_decl (gpio, sunxi_gpio, gpio0);
dev_decl (gpio, sunxi_gpio, gpio1);

drv_incl (pic, arm_gic);
dev_decl (pic, arm_gic, pic);
drv_incl (timer, sunxi_timer);
dev_decl (timer, sunxi_timer, timer0);
dev_decl (timer, sunxi_timer, timer1);

drv_incl (block, sunxi_mmc);
dev_decl (block, sunxi_mmc, mmcblk0);
drv_incl (block, mbr);
dev_decl (block, mbr, mmcblk0p1);

drv_incl (fs, fat32);
dev_decl (fs, fat32, root);

extern void
devtree_init(void)
{
    APB0_GATE = 1;

    /* Serial */
    dev_init (tty0, 0x01c28000);
    dev_init (tty1, 0x01c28400);
    dev_init (tty2, 0x01c28800);
    dev_init (tty3, 0x01c28c00);
    dev_init (tty4, 0x01f02800);
    uart_config(&dev(tty0), 115200, UART_8B, UART_NOPARITY, UART_1S);
    uart_config(&dev(tty1), 115200, UART_8B, UART_NOPARITY, UART_1S);

    /* Interrupts */
    dev_init (pic, 0x01c82000, 0x01c81000);

    /* GPIO initialization */
    dev_init (gpio0, 0x01c20800, 6, 2, &dev(pic), (u16[]){43, 49});
    dev_init (gpio1, 0x01f02c00, 1, 1, &dev(pic), (u16[]){77});

    switch (CONFIG_SUNXI_BOARD)
    {
        case ORANGEPI_ONE:
            gpio_config(&dev(gpio1), 10, GPIO_OUT, GPIO_PULLOFF);
            gpio_set(&dev(gpio1), 10, true);
            break;

        case NANOPI_NEO:
            gpio_config(&dev(gpio0), 10, GPIO_OUT, GPIO_PULLOFF);
            gpio_set(&dev(gpio0), 10, true);
            break;
    }

    /* Timers */
    dev_init (timer0, 0x01c20c00, 0, &dev(pic), 50);
    dev_init (timer1, 0x01c20c00, 1, &dev(pic), 51);

    /* Storage */
    dev_init (mmcblk0,   0x01c0f000);
    dev_init (mmcblk0p1, &dev(mmcblk0), 1);
    dev_init (root, &dev(mmcblk0p1));

    /* Systems */
    comm_config((dev_stream *)&dev(tty0), (dev_stream *)&dev(tty0),
                (dev_stream *)&dev(tty1), (dev_stream *)&dev(tty1));
    pic_state(&dev(pic), true);
    wheel_timer(&dev(timer0));
}

extern void
devtree_clean(void)
{
    dev_clean (tty2);
    dev_clean (tty3);
    dev_clean (tty4);

    gpio_set(&dev(gpio1), 10, false);
    gpio_config(&dev(gpio1), 10, GPIO_OFF, GPIO_PULLOFF);

    gpio_config(&dev(gpio0), 15, GPIO_OUT, GPIO_PULLOFF);
    gpio_set(&dev(gpio0), 15, true);

    dev_clean (gpio0);
    dev_clean (gpio1);

    dev_clean (timer0);
    dev_clean (timer1);

    dev_clean (mmcblk0);
    dev_clean (mmcblk0p1);

    dev_clean (root);

    dev_clean (tty0);
    dev_clean (tty1);
    comm_config(NULL, NULL, NULL, NULL);

    APB0_GATE = 0;
}
