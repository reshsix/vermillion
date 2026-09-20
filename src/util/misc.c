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

#include <vermillion/hal/spi.h>
#include <vermillion/hal/uart.h>
#include <vermillion/hal/timer.h>

#include <vermillion/sys/task.h>

extern uint8_t
vrm_misc_getc(uint8_t id)
{
    uint8_t data = 0;

    while (!vrm_uart_read(id, &data, 0));

    return data;
}

extern void
vrm_misc_putc(uint8_t id, uint8_t data)
{
    while (!vrm_uart_write(id, data, 0));
}

extern void
vrm_misc_spi(uint8_t id, uint8_t *data, size_t count, uint32_t flags)
{
    size_t limit = 0;
    while (!vrm_spi_limit(id, &limit))
        vrm_task_yield();

    uint32_t flags2 = flags | VRM_SPI_PARTIAL;
    for (size_t i = 0; i < count; i += limit)
    {
        if (i + limit >= count)
            flags2 = flags;

        size_t remain = count - i;
        size_t size = (remain > limit) ? limit : remain;
        while (!vrm_spi_transfer(id, &(data[i]), size, flags2));
        while (!vrm_spi_poll(id));
    }
}

static void
sleep(void *arg)
{
    bool *flag = arg;
    *flag = true;
}

extern void
vrm_misc_sleep(uint8_t id, uint32_t us)
{
    volatile bool flag = false;

    if (vrm_timer_alarm(id, us, false, sleep, (bool *)&flag))
    {
        while (!flag);
    }
}
