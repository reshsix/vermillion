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
#include <vermillion/task.h>
#include <vermillion/devtree.h>
#include <vermillion/hal/spi.h>
#include <vermillion/hal/disk.h>
#include <vermillion/hal/gpio.h>
#include <vermillion/hal/uart.h>
#include <vermillion/hal/timer.h>
#include <vermillion/sys/file.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/debug.h>
#include <vermillion/util/types.h>

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile uint32_t*)(R_PRCM + 0x28)

#define CCU 0x01C20000
#define CLK_SPI0   *(volatile uint32_t*)(CCU + 0xA0)
#define BUS0_GATE  *(volatile uint32_t*)(CCU + 0x60)
#define BUS3_GATE  *(volatile uint32_t*)(CCU + 0x6C)
#define BUS0_RESET *(volatile uint32_t*)(CCU + 0x2C0)
#define BUS4_RESET *(volatile uint32_t*)(CCU + 0x2D8)

dev_fs    fs   [1];
dev_spi   spi  [1];
dev_gpio  gpio [2];
dev_uart  uart [3];
dev_disk  disk [2];
dev_timer timer[2];

struct vrm_task_mut spi_mut  [1] = {0};
struct vrm_task_mut gpio_mut [2] = {0};
struct vrm_task_mut uart_mut [3] = {0};
struct vrm_task_mut disk_mut [2] = {0};
struct vrm_task_mut timer_mut[2] = {0};

struct led
{
    uint8_t id, port, slot;
};

struct led power  = {0};
struct led status = {0};

static uint64_t
check_init(void *ptr, uint64_t flag, const char *s)
{
    uint64_t ret = 0;

    if (ptr)
        ret = flag;
    else
        vrm_debug("Failed to init ~s", s);

    return ret;
}

#define INIT_DEV(flag, x) \
    for (int _once##__LINE__ = 1; \
             _once##__LINE__ && (flags & flag); \
            (_once##__LINE__ = 0, ret |= check_init(x.context, flag, #x)))

#define TRY(name, x, y, n, ...) \
    bool name##_##x##_##y##_success = false; \
    if (x[n].context) \
    { \
        for (int i = 0; !(name##_##x##_##y##_success)  && i < 100; i++) \
            name##_##x##_##y##_success = vrm_##x##_##y(i, __VA_ARGS__); \
    \
        if (!(name##_##x##_##y##_success)) \
            vrm_debug("Failed to ~s ~s", #y, #name); \
    } \
    else \
        vrm_debug("Failed to ~s ~s", #y, #name);

static uint64_t sflags = 0;
extern uint64_t
vrm_devtree_init(uint8_t platform, uint8_t board, uint64_t flags)
{
    uint64_t ret = 0;

    if (platform == VRM_PLATFORM_SUNXI_H3)
    {
        mem_init();
        APB0_GATE = 1;

        /* Set pointers and mutexes */
        file_setup (fs,               1);
        spi_setup  (spi,   spi_mut,   1);
        gpio_setup (gpio,  gpio_mut,  2);
        uart_setup (uart,  uart_mut,  3);
        disk_setup (disk,  disk_mut,  2);
        timer_setup(timer, timer_mut, 2);

        /* ARM GIC */
        gic_init(0x01c82000, 0x01c81000);

        /* Debug UART */
        INIT_DEV(VRM_FLAG_UART0, uart[0])
        {
            uart[0] = sunxi_uart_init(0);
            TRY(UART0, uart, config,
                0, 115200, VRM_UART_8B | VRM_UART_NONE | VRM_UART_1S);
        }

        /* Peripherals */
        INIT_DEV(VRM_FLAG_GPIO0, gpio[0])
            gpio[0] = sunxi_gpio_init(0);
        INIT_DEV(VRM_FLAG_GPIO1, gpio[1])
            gpio[1] = sunxi_gpio_init(1);
        INIT_DEV(VRM_FLAG_UART1, uart[1])
        {
            TRY(PG6, gpio, config, 0, 6, 6, VRM_GPIO_MUX0);
            TRY(PG7, gpio, config, 0, 6, 7, VRM_GPIO_MUX0);
            TRY(PG8, gpio, config, 0, 6, 8, VRM_GPIO_MUX0);
            TRY(PG9, gpio, config, 0, 6, 9, VRM_GPIO_MUX0);

            BUS3_GATE  |= 1 << 17;
            BUS4_RESET |= 1 << 17;

            uart[1] = sunxi_uart_init(1);
            TRY(UART1, uart, config,
                1, 115200, VRM_UART_8B | VRM_UART_NONE | VRM_UART_1S);
        }
        INIT_DEV(VRM_FLAG_UART2, uart[2])
        {
            TRY(PA0, gpio, config, 0, 0, 0, VRM_GPIO_MUX0);
            TRY(PA1, gpio, config, 0, 0, 1, VRM_GPIO_MUX0);
            TRY(PA2, gpio, config, 0, 0, 2, VRM_GPIO_MUX0);
            TRY(PA3, gpio, config, 0, 0, 3, VRM_GPIO_MUX0);

            BUS3_GATE  |= 1 << 18;
            BUS4_RESET |= 1 << 18;

            uart[2] = sunxi_uart_init(2);
            TRY(UART2, uart, config,
                2, 115200, VRM_UART_8B | VRM_UART_NONE | VRM_UART_1S);
        }
        INIT_DEV(VRM_FLAG_SPI0, spi[0])
        {
            TRY(PC0, gpio, config, 0, 2, 0, VRM_GPIO_MUX1);
            TRY(PC1, gpio, config, 0, 2, 1, VRM_GPIO_MUX1);
            TRY(PC2, gpio, config, 0, 2, 2, VRM_GPIO_MUX1);
            TRY(PC3, gpio, config, 0, 2, 3, VRM_GPIO_MUX1);

            CLK_SPI0    = 1 << 31;
            BUS0_GATE  |= 1 << 20;
            BUS0_RESET |= 1 << 20;

            spi[0] = sunxi_spi_init(0);
            TRY(SPI0, spi, config,
                0, 24000000, VRM_SPI_MODE0 | VRM_SPI_MSB | VRM_SPI_CSL);
        }

        /* Board LEDs */
        if ((flags & VRM_FLAG_LEDS))
        {
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

            /* Power led ON */
            TRY(LED_PWR, gpio, config,
                power.id, power.port, power.slot, VRM_GPIO_OUT);
            TRY(LED_PWR, gpio, set,
                power.id, power.port, power.slot, true);
            /* Status led OFF */
            TRY(LED_STAT, gpio, config,
                status.id, status.port, status.slot, VRM_GPIO_OUT);
            TRY(LED_STAT, gpio, set,
                status.id, status.port, status.slot, false);

            if ( LED_PWR_gpio_config_success && LED_PWR_gpio_set_success &&
                LED_STAT_gpio_config_success && LED_STAT_gpio_set_success)
                ret |= VRM_FLAG_LEDS;
        }

        /* Timers */
        INIT_DEV(VRM_FLAG_TIMER0, timer[0])
            timer[0] = sunxi_timer_init(0);
        INIT_DEV(VRM_FLAG_TIMER1, timer[1])
            timer[1] = sunxi_timer_init(1);

        /* Disks */
        INIT_DEV(VRM_FLAG_SDCARD, fs[0])
        {
            disk[0] = sunxi_mmc_init(0);
            if (disk[0].context)
                disk[1] = mbr_init(0, 0, 1);
            if (disk[1].context)
                fs[0] = fat32_init(0, 1);
        }

        /* Interrupts ON */
        gic_state(true);

        /* Save the result statically */
        sflags = ret;

        /* If VRM_FLAG_ALL, don't complain about unknown flags */
        if (flags & (1ULL << 63))
            ret |= flags & ~0x3FFULL;
    }

    return ret;
}

extern void
vrm_devtree_clean(void)
{
    /* Interrupts OFF */
    gic_state(false);

    /* Board LEDs */
    if ((sflags & VRM_FLAG_LEDS))
    {
        /* Power led OFF */
        TRY(LED_PWR, gpio, set,
            power.id, power.port, power.slot, false);
        /* Status led ON */
        TRY(LED_STAT, gpio, set,
            status.id, status.port, status.slot, true);
    }

    /* Peripherals clean */
    if ((sflags & VRM_FLAG_GPIO0))
        sunxi_gpio_clean(&(gpio[0]));
    if ((sflags & VRM_FLAG_GPIO1))
        sunxi_gpio_clean(&(gpio[1]));
    if ((sflags & VRM_FLAG_UART1))
        sunxi_uart_clean(&(uart[1]));
    if ((sflags & VRM_FLAG_UART2))
        sunxi_uart_clean(&(uart[2]));
    if ((sflags & VRM_FLAG_SPI0))
        sunxi_spi_clean(&(spi[0]));

    /* Timer clean */
    if ((sflags & VRM_FLAG_TIMER0))
        sunxi_timer_clean(&(timer[0]));
    if ((sflags & VRM_FLAG_TIMER1))
        sunxi_timer_clean(&(timer[1]));

    /* Disks clean */
    if ((sflags & VRM_FLAG_SDCARD))
    {
        fat32_clean(&(fs[0]));
        mbr_clean(&(disk[1]));
        sunxi_mmc_clean(&(disk[0]));
    }

    /* Debug UART clean */
    sunxi_uart_clean(&(uart[0]));

    /* Interrupts clean */
    gic_clean();

    APB0_GATE = 0;
    mem_clean();
}
