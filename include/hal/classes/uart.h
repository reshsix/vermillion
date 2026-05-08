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

#pragma once

#include <hal/stream.h>

typedef drv_stream drv_uart;
typedef dev_stream dev_uart;

/* For driver usage */

enum uart_index
{
    UART_BAUD_GET, UART_BAUD_SET
};

/* For devtree usage */

void uart_setup(dev_uart *list, u8 count);

/* For external usage*/

bool uart_read(u8 id, u8 *data);
bool uart_write(u8 id, u8 data);
bool uart_info(u8 id, u32 *baud);
bool uart_config(u8 id, u32 baud);
