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

#pragma once

#include <hal/stream.h>

enum uart_index
{
    UART_CONFIG = STREAM_COMMON + 1,
};

typedef drv_stream drv_uart;
typedef dev_stream dev_uart;

enum uart_bits
{
    UART_5B, UART_6B, UART_7B, UART_8B
};

enum uart_parity
{
    UART_NOPARITY, UART_ODD, UART_EVEN, UART_MARK, UART_SPACE
};

enum uart_stop
{
    UART_1S, UART_1HS, UART_2S
};

struct [[gnu::packed]] uart_cfg
{
    u32 baud;
    enum uart_bits bits;
    enum uart_parity parity;
    enum uart_stop stop;
};

bool uart_read(dev_uart *du, u8 *data);
bool uart_write(dev_uart *du, u8 data);
bool uart_info(dev_uart *du, u32 *baud, enum uart_bits *bits,
               enum uart_parity *parity, enum uart_stop *stop);
bool uart_config(dev_uart *du, u32 baud, enum uart_bits bits,
                 enum uart_parity parity, enum uart_stop stop);
