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

#include <vermillion/lib.h>
#include <vermillion/vrm.h>
#include <vermillion/prog.h>

#include <vermillion/hal/uart.h>
static struct vrm_uart_v1 *uart = NULL;

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    bool ret = true;

    uart = v->driver(VRM_UART, VRM_UART_V1);
    uart->write(0, 'a');

    return ret;
}
