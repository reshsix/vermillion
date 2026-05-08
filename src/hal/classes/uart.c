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

#include <general/types.h>

#include <hal/classes/uart.h>

/* For devtree usage */

static dev_uart *dev_l = NULL;
static u8 dev_c = 0;

extern void
uart_setup(dev_uart *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* For external usage */

static void *
uart_dev(u8 id)
{
    return (id < dev_c) ? &(dev_l[id]) : NULL;
}

extern bool
uart_read(u8 id, u8 *data)
{
    return stream_read(uart_dev(id), data);
}

extern bool
uart_write(u8 id, u8 data)
{
    return stream_write(uart_dev(id), &data);
}

extern bool
uart_info(u8 id, u32 *baud)
{
    u32 baud2 = 0;

    bool ret = stream_ioctl(uart_dev(id), UART_BAUD_GET, &baud2);
    if (ret && baud)
        *baud = baud2;

    return ret;
}

extern bool
uart_config(u8 id, u32 baud)
{
    if (baud == 0)
        baud = 115200;

    return stream_ioctl(uart_dev(id), UART_BAUD_SET, &baud);
}
