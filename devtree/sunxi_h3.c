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
#include <drivers/arm/sunxi/spi.h>
#include <drivers/arm/sunxi/gpio.h>
#include <drivers/arm/sunxi/uart.h>
#include <drivers/arm/sunxi/timer.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)

#define CCU 0x01C20000
#define CLK_SPI0   *(volatile u32*)(CCU + 0xA0)
#define BUS0_GATE  *(volatile u32*)(CCU + 0x60)
#define BUS0_RESET *(volatile u32*)(CCU + 0x2C0)

#define ORANGEPI_ONE 0
#define NANOPI_NEO 1

dev_fs root;
dev_pic pic;
dev_spi spi0;
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
    tty0 = sunxi_uart_init(0);
    tty1 = sunxi_uart_init(1);
    tty2 = sunxi_uart_init(2);
    tty3 = sunxi_uart_init(3);
    tty4 = sunxi_uart_init(4);
    uart_config(&tty0, 115200, UART_8B, UART_NOPARITY, UART_1S);

    /* Interrupts */
    pic = arm_gic_init(0x01c82000, 0x01c81000);

    /* GPIO initialization */
    gpio0 = sunxi_gpio_init(0, &pic);
    gpio1 = sunxi_gpio_init(1, &pic);
    switch (CONFIG_SUNXI_BOARD)
    {
        /* Green led on PL10 */
        case ORANGEPI_ONE:
            gpio_config(&gpio1, 10, GPIO_OUT, GPIO_PULLOFF);
            gpio_set(&gpio1, 10, true);
            break;
        /* Green led on PA10 */
        case NANOPI_NEO:
            gpio_config(&gpio0, 10, GPIO_OUT, GPIO_PULLOFF);
            gpio_set(&gpio0, 10, true);
            break;
    }
    /* UART1 on PG6 to PG9 */
    gpio_config(&gpio0, (6 * 32) + 6, GPIO_CUSTOM + 0, GPIO_PULLOFF);
    gpio_config(&gpio0, (6 * 32) + 7, GPIO_CUSTOM + 0, GPIO_PULLOFF);
    gpio_config(&gpio0, (6 * 32) + 8, GPIO_CUSTOM + 0, GPIO_PULLOFF);
    gpio_config(&gpio0, (6 * 32) + 9, GPIO_CUSTOM + 0, GPIO_PULLOFF);
    /* SPI0 on PC0 to PC3 */
    gpio_config(&gpio0, (2 * 32) + 0, GPIO_CUSTOM + 1, GPIO_PULLOFF);
    gpio_config(&gpio0, (2 * 32) + 1, GPIO_CUSTOM + 1, GPIO_PULLOFF);
    gpio_config(&gpio0, (2 * 32) + 2, GPIO_CUSTOM + 1, GPIO_PULLOFF);
    gpio_config(&gpio0, (2 * 32) + 3, GPIO_CUSTOM + 1, GPIO_PULLOFF);

    /* Timers */
    timer0 = sunxi_timer_init(0, &pic);
    timer1 = sunxi_timer_init(1, &pic);

    /* Storage */
    mmcblk0 = sunxi_mmc_init(0);
    mmcblk0p1 = mbr_init(0, &mmcblk0, 1);
    root = fat32_init(&mmcblk0p1);

    /* Peripherals */
    CLK_SPI0    = 1 << 31;
    BUS0_GATE  |= 1 << 20;
    BUS0_RESET |= 1 << 20;
    spi0 = sunxi_spi_init(0);
    spi_config(&spi0, 24000000, SPI_MODE0, false, false, false);

    /* Systems */
    comm_setup(&tty0, &tty1, &gpio0, (uint16_t[]){0,1,2,3}, 4, &spi0);
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

    comm_setup(NULL, NULL, NULL, NULL, 0, NULL);
    sunxi_uart_clean(&tty0);
    sunxi_uart_clean(&tty1);
    sunxi_uart_clean(&tty2);
    sunxi_uart_clean(&tty3);
    sunxi_uart_clean(&tty4);
    sunxi_spi_clean(&spi0);

    arm_gic_clean(&pic);

    APB0_GATE = 0;
    mem_clean();
}
