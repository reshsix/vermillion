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

#include <core/types.h>

#include <core/uart.h>
#include <core/stream.h>

extern bool
uart_info(dev_uart *du, u32 *baud, enum uart_bits *bits,
          enum uart_parity *parity, enum uart_stop *stop)
{
    struct uart_cfg cfg = {0};

    bool ret = stream_read((dev_stream *)du, 0, &cfg);
    if (ret)
    {
        if (baud)   *baud   = cfg.baud;
        if (bits)   *bits   = cfg.bits;
        if (parity) *parity = cfg.parity;
        if (stop)   *stop   = cfg.stop;
    }

    return ret;
}

extern bool
uart_config(dev_uart *du, u32 baud, enum uart_bits bits,
            enum uart_parity parity, enum uart_stop stop)
{
    struct uart_cfg cfg = {.baud = baud, .bits = bits,
                           .parity = parity, stop = stop};

    return stream_write((dev_stream *)du, 0, &cfg);
}

extern bool
uart_read(dev_uart *du, u8 *data)
{
    return stream_read((dev_stream *)du, 1, data);
}

extern bool
uart_write(dev_uart *du, u8 data)
{
    return stream_write((dev_stream *)du, 1, &data);
}
