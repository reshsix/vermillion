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

#define VERMILLION_INTERNALS
#include <vermillion/hal/uart.h>

/* Devtree setup */

static dev_uart *dev_l = NULL;
static u8 dev_c = 0;

extern void
uart_setup(dev_uart *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define UART_CALL(f, ...) \
(id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false;

extern bool
uart_read(u8 id, u8 *data)
{
    return UART_CALL(read, data);
}

extern bool
uart_write(u8 id, u8 data)
{
    return UART_CALL(write, data);
}

extern bool
uart_info(u8 id, u32 *baud)
{
    u32 baud2 = 0;

    bool ret = UART_CALL(info, &baud2);
    if (ret && baud)
        *baud = baud2;

    return ret;
}

extern bool
uart_config(u8 id, u32 baud)
{
    if (baud == 0)
        baud = 115200;

    return UART_CALL(config, baud);
}

/* ABI definitions */

static struct vrm_uart_v1 v1 =
{
    .read = uart_read, .write  = uart_write,
    .info = uart_info, .config = uart_config
};

extern void *
uart_driver(u8 version)
{
    void *ret = NULL;

    switch (version)
    {
        case VRM_UART_V1:
            ret = &v1;
            break;
    }

    return ret;
}
