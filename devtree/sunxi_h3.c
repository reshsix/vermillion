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

#include <general/mem.h>
#include <general/types.h>

#include <hal/block.h>
#include <hal/stream.h>
#include <hal/classes/fs.h>
#include <hal/classes/pic.h>
#include <hal/classes/gpio.h>
#include <hal/classes/uart.h>
#include <hal/classes/timer.h>

#include <system/comm.h>
#include <system/disk.h>
#include <system/time.h>

#include <drivers/fs/mbr.h>
#include <drivers/fs/fat32.h>
#include <drivers/arm/gic.h>
#include <drivers/arm/sunxi/mmc.h>
#include <drivers/arm/sunxi/gpio.h>
#include <drivers/arm/sunxi/uart.h>
#include <drivers/arm/sunxi/timer.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

#define ORANGEPI_ONE 0
#define NANOPI_NEO 1

dev_fs root;
dev_pic pic;
dev_gpio gpio0, gpio1;
dev_uart tty0, tty1, tty2, tty3, tty4;
dev_block mmcblk0, mmcblk0p1;
dev_timer timer0, timer1;

extern void
devtree_init(void)
{
    mem_init();
    APB0_GATE = 1;

    /* Serial */
    tty0 = sunxi_uart_init(0x01c28000);
    tty1 = sunxi_uart_init(0x01c28400);
    tty2 = sunxi_uart_init(0x01c28800);
    tty3 = sunxi_uart_init(0x01c28c00);
    tty4 = sunxi_uart_init(0x01f02800);
    uart_config(&tty0, 115200, UART_8B, UART_NOPARITY, UART_1S);
    uart_config(&tty1, 115200, UART_8B, UART_NOPARITY, UART_1S);

    /* Interrupts */
    pic = arm_gic_init(0x01c82000, 0x01c81000);

    /* GPIO initialization */
    gpio0 = sunxi_gpio_init(0x01c20800, 6, 2, &pic, (u16[]){43, 49});
    gpio1 = sunxi_gpio_init(0x01f02c00, 1, 1, &pic, (u16[]){77});
    switch (CONFIG_SUNXI_BOARD)
    {
        case ORANGEPI_ONE:
            gpio_config(&gpio1, 10, GPIO_OUT, GPIO_PULLOFF);
            gpio_set(&gpio1, 10, true);
            break;

        case NANOPI_NEO:
            gpio_config(&gpio0, 10, GPIO_OUT, GPIO_PULLOFF);
            gpio_set(&gpio0, 10, true);
            break;
    }

    /* Timers */
    timer0 = sunxi_timer_init(0x01c20c00, 0, &pic, 50);
    timer1 = sunxi_timer_init(0x01c20c00, 1, &pic, 51);

    /* Storage */
    mmcblk0 = sunxi_mmc_init(0x01c0f000);
    mmcblk0p1 = mbr_init(&mmcblk0, 1);
    root = fat32_init(&mmcblk0p1);

    /* Systems */
    comm_config(&tty0, &tty0, &tty1, &tty1);
    disk_config(&root);
    time_config(&timer0);
    pic_state(&pic, true);
}

extern void
devtree_clean(void)
{
    pic_state(&pic, false);

    gpio_set(&gpio1, 10, false);
    gpio_config(&gpio1, 10, GPIO_OFF, GPIO_PULLOFF);

    gpio_config(&gpio0, 15, GPIO_OUT, GPIO_PULLOFF);
    gpio_set(&gpio0, 15, true);

    sunxi_gpio_clean(&gpio0);
    sunxi_gpio_clean(&gpio1);

    sunxi_timer_clean(&timer0);
    sunxi_timer_clean(&timer1);

    sunxi_mmc_clean(&mmcblk0);
    mbr_clean(&mmcblk0p1);

    fat32_clean(&root);

    comm_config(NULL, NULL, NULL, NULL);
    sunxi_uart_clean(&tty0);
    sunxi_uart_clean(&tty1);
    sunxi_uart_clean(&tty2);
    sunxi_uart_clean(&tty3);
    sunxi_uart_clean(&tty4);

    arm_gic_clean(&pic);

    APB0_GATE = 0;
    mem_clean();
}
