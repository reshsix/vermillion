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

#define VERMILLION_INTERNALS
#include <vermillion/task.h>
#include <vermillion/hal/uart.h>
#include <vermillion/util/types.h>

/* Devtree setup */

static dev_uart            *dev_l = NULL;
static struct vrm_task_mut *dev_m = NULL;
static uint8_t              dev_c = 0;

extern void
uart_setup(dev_uart *list, struct vrm_task_mut *muts, uint8_t count)
{
    dev_l = list;
    dev_m = muts;
    dev_c = count;
}

/* Driver calls */

#define UART_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
vrm_uart_info(uint8_t id, uint32_t *baud, uint32_t *fields)
{
    bool ret = false;

    uint32_t baud2 = 0, fields2 = 0;
    VRM_TASK_MUTEX(&(dev_m[id]), 0)
    {
        ret = UART_CALL(info, &baud2, &fields2);
        if (ret)
        {
            if (baud)
                *baud = baud2;
            if (fields)
                *fields = fields2;
        }
    }

    return ret;
}

extern bool
vrm_uart_config(uint8_t id, uint32_t baud, uint32_t fields)
{
    bool ret = false;

    baud = (baud != 0) ? baud : 115200;
    VRM_TASK_MUTEX(&(dev_m[id]), 0)
        ret = UART_CALL(config, baud, fields);

    return ret;
}

extern bool
vrm_uart_read(uint8_t id, uint8_t *data, uint32_t flags)
{
    bool ret = false;

    (void)flags;
    VRM_TASK_MUTEX(&(dev_m[id]), 0)
        ret = UART_CALL(read, data);

    return ret;
}

extern bool
vrm_uart_write(uint8_t id, uint8_t data, uint32_t flags)
{
    bool ret = false;

    (void)flags;
    VRM_TASK_MUTEX(&(dev_m[id]), 0)
        ret = UART_CALL(write, data);

    return ret;
}
