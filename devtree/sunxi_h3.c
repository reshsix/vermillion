/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <arch/gic.h>

#include <drivers/fs/mbr.h>
#include <drivers/fs/fat32.h>
#include <drivers/arm/sunxi/mmc.h>
#include <drivers/arm/sunxi/spi.h>
#include <drivers/arm/sunxi/gpio.h>
#include <drivers/arm/sunxi/uart.h>
#include <drivers/arm/sunxi/timer.h>

#define VERMILLION_INTERNALS
#include <vermillion/devtree.h>
#include <vermillion/hal/spi.h>
#include <vermillion/hal/disk.h>
#include <vermillion/hal/gpio.h>
#include <vermillion/hal/uart.h>
#include <vermillion/hal/timer.h>
#include <vermillion/sys/file.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile uint32_t*)(R_PRCM + 0x28)

#define CCU 0x01C20000
#define CLK_SPI0   *(volatile uint32_t*)(CCU + 0xA0)
#define BUS0_GATE  *(volatile uint32_t*)(CCU + 0x60)
#define BUS3_GATE  *(volatile uint32_t*)(CCU + 0x6C)
#define BUS0_RESET *(volatile uint32_t*)(CCU + 0x2C0)
#define BUS4_RESET *(volatile uint32_t*)(CCU + 0x2D8)

#define ORANGEPI_ONE 0
#define NANOPI_NEO 1

dev_fs    fs   [1];
dev_spi   spi  [1];
dev_gpio  gpio [2];
dev_uart  uart [3];
dev_disk  disk [2];
dev_timer timer[2];

struct led
{
    uint8_t id, port, slot;
};
struct led power  = {0};
struct led status = {0};

extern bool
vrm_devtree_init(uint8_t platform, uint8_t board, uint32_t flags)
{
    bool ret = (platform == VRM_PLATFORM_SUNXI_H3);

    (void)flags;
    if (ret)
    {
        mem_init();
        APB0_GATE = 1;

        switch (board)
        {
            case VRM_BOARD_ORANGEPI_ONE:
                power.id    = 1;
                power.port  = 0;
                power.slot  = 10;
                status.id   = 0;
                status.port = 0;
                status.slot = 15;
                break;
            case VRM_BOARD_NANOPI_NEO:
                power.id    = 0;
                power.port  = 0;
                power.slot  = 10;
                status.id   = 1;
                status.port = 0;
                status.slot = 10;
                break;
        }

        /* Interrupts */
        gic_init(0x01c82000, 0x01c81000);

        /* Serial */
        uart[0] = sunxi_uart_init(0);
        BUS3_GATE  |= 1 << 17;
        BUS4_RESET |= 1 << 17;
        BUS3_GATE  |= 1 << 18;
        BUS4_RESET |= 1 << 18;
        uart[1] = sunxi_uart_init(1);
        uart[2] = sunxi_uart_init(2);
        uart_setup(uart, 3);
        vrm_uart_config(0, 115200, VRM_UART_8B | VRM_UART_NONE | VRM_UART_1S);
        vrm_uart_config(1, 115200, VRM_UART_8B | VRM_UART_NONE | VRM_UART_1S);
        vrm_uart_config(2, 115200, VRM_UART_8B | VRM_UART_NONE | VRM_UART_1S);

        /* GPIO initialization */
        gpio[0] = sunxi_gpio_init(0);
        gpio[1] = sunxi_gpio_init(1);
        gpio_setup(gpio, 2);
        /* Power led ON */
        vrm_gpio_config(power.id, power.port, power.slot, VRM_GPIO_OUT);
        vrm_gpio_set(power.id, power.port, power.slot, true);
        /* Status led OFF */
        vrm_gpio_config(status.id, status.port, status.slot, VRM_GPIO_OUT);
        vrm_gpio_set(status.id, status.port, status.slot, false);
        /* UART1 on PG6 to PG9 */
        vrm_gpio_config(0, 6, 6, VRM_GPIO_MUX0);
        vrm_gpio_config(0, 6, 7, VRM_GPIO_MUX0);
        vrm_gpio_config(0, 6, 8, VRM_GPIO_MUX0);
        vrm_gpio_config(0, 6, 9, VRM_GPIO_MUX0);
        /* UART2 on PA0 to PA3 */
        vrm_gpio_config(0, 0, 0, VRM_GPIO_MUX0);
        vrm_gpio_config(0, 0, 1, VRM_GPIO_MUX0);
        vrm_gpio_config(0, 0, 2, VRM_GPIO_MUX0);
        vrm_gpio_config(0, 0, 3, VRM_GPIO_MUX0);
        /* SPI0 on PC0 to PC3 */
        vrm_gpio_config(0, 2, 0, VRM_GPIO_MUX1);
        vrm_gpio_config(0, 2, 1, VRM_GPIO_MUX1);
        vrm_gpio_config(0, 2, 2, VRM_GPIO_MUX1);
        vrm_gpio_config(0, 2, 3, VRM_GPIO_MUX1);

        /* Timers */
        timer[0] = sunxi_timer_init(0);
        timer[1] = sunxi_timer_init(1);
        timer_setup(timer, 2);

        /* Disks */
        disk[0] = sunxi_mmc_init(0);
        disk_setup(disk, 1);
        disk[1] = mbr_init(0, 0, 1);
        disk_setup(disk, 2);

        /* Filesystems */
        fs[0] = fat32_init(1);
        file_setup(fs, 1);

        /* Peripherals */
        CLK_SPI0    = 1 << 31;
        BUS0_GATE  |= 1 << 20;
        BUS0_RESET |= 1 << 20;
        spi[0] = sunxi_spi_init(0);
        spi_setup(spi, 1);
        vrm_spi_config(0, 24000000, VRM_SPI_MODE0 | VRM_SPI_MSB | VRM_SPI_CSL);

        /* Interrupts ON */
        gic_state(true);
    }

    return ret;
}

extern void
vrm_devtree_clean(void)
{
    /* Interrupts OFF */
    gic_state(false);

    /* Power led OFF */
    vrm_gpio_config(power.id, power.port, power.slot, VRM_GPIO_OUT);
    vrm_gpio_set(power.id, power.port, power.slot, false);
    /* Status led ON */
    vrm_gpio_config(status.id, status.port, status.slot, VRM_GPIO_OUT);
    vrm_gpio_set(status.id, status.port, status.slot, true);

    /* GPIO clean */
    sunxi_gpio_clean(&(gpio[0]));
    sunxi_gpio_clean(&(gpio[1]));

    /* Timer clean */
    sunxi_timer_clean(&(timer[0]));
    sunxi_timer_clean(&(timer[1]));

    /* Storage clean */
    fat32_clean(&(fs[0]));
    mbr_clean(&(disk[1]));
    sunxi_mmc_clean(&(disk[0]));

    /* Peripherals clean */
    sunxi_spi_clean(&(spi[0]));

    /* Serial clean */
    sunxi_uart_clean(&(uart[0]));
    sunxi_uart_clean(&(uart[1]));

    /* Interrupts clean */
    gic_clean();

    APB0_GATE = 0;
    mem_clean();
}
