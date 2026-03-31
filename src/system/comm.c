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

#include <general/types.h>

#include <hal/stream.h>
#include <hal/classes/spi.h>
#include <hal/classes/uart.h>

#include <system/comm.h>

static dev_stream *u0  = NULL;
static dev_stream *u1  = NULL;
static dev_spi    *spi0 = NULL;

/* For devtree usage */

extern void
comm_setup(dev_stream *uart0, dev_stream *uart1,
           dev_spi *spi)
{
    u0   = uart0;
    u1   = uart1;
    spi0 = spi;
}

/* For external usage */

extern uint32_t
comm_flags_uart(uint8_t bits, uint8_t parity, uint8_t stop)
{
    return bits | (parity << 4) | (stop << 8);
}

extern uint32_t
comm_flags_spi(uint8_t mode, bool lsb, bool csp, bool duplex)
{
    return mode | (lsb << 4) | (csp << 5) | (duplex << 6);
}

extern bool
comm_info(uint8_t id, uint32_t *rate, uint32_t *flags)
{
    bool ret = false;

    uint32_t rate2  = 0;
    uint32_t flags2 = 0;
    switch (id)
    {
        case COMM_UART0:
        case COMM_UART1:;
            enum uart_bits   bits;
            enum uart_parity parity;
            enum uart_stop   stop;

            ret = uart_info((id == COMM_UART0) ? u0 : u1, &rate2,
                            &bits, &parity, &stop);
            if (ret)
                flags2 = bits | (parity << 4) | (stop << 8);
            break;
        case COMM_SPI:
            enum spi_mode mode;
            bool lsb, csp, duplex;

            ret = spi_info(spi0, rate, &mode, &lsb, &csp, &duplex);
            if (ret)
                flags2 = mode | (lsb << 4) | (csp << 5) | (duplex << 6);
            break;
    }

    return ret;
}

extern bool
comm_config(uint8_t id, uint32_t rate, uint32_t flags)
{
    bool ret = false;

    switch (id)
    {
        case COMM_UART0:
        case COMM_UART1:
            ret = uart_config((id == COMM_UART0) ? u0 : u1, rate,
                              (flags & 0x0F)   >> 0,
                              (flags & 0xF0)   >> 4,
                              (flags & 0x0F00) >> 8);
            break;
        case COMM_SPI:
            ret = spi_config(spi0, rate, flags & 0x0F,
                             flags & 0x10, flags & 0x20, flags & 0x40);
            break;
    }

    return ret;
}

extern bool
comm_read(uint8_t id, char *c)
{
    bool ret = false;

    switch (id)
    {
        case COMM_UART0:
        case COMM_UART1:
            ret = stream_read((id == COMM_UART0) ? u0 : u1, STREAM_COMMON, c);
            break;
        case COMM_SPI:
            ret = spi_read(spi0, (u8*)c);
            break;
    }

    return ret;
}

extern bool
comm_write(uint8_t id, char c)
{
    bool ret = false;

    switch (id)
    {
        case COMM_UART0:
        case COMM_UART1:
            ret = stream_write((id == COMM_UART0) ? u0 : u1,
                               STREAM_COMMON, &c);
            break;
        case COMM_SPI:
            ret = spi_write(spi0, (u8*)&c);
            break;
    }

    return ret;
}
